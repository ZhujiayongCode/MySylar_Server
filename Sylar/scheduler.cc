#include"scheduler.h"
#include"log.h"
#include"macro.h"
#include"hook.h"

namespace Sylar{

    static Sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    /**
     * @brief 线程局部变量，指向当前线程所使用的调度器实例。
     * @details 每个线程都有自己独立的 t_scheduler 变量，初始值为 nullptr。
     *          当线程开始使用某个调度器时，该变量会被设置为对应的调度器指针。
     */
    static thread_local Scheduler*t_scheduler=nullptr;
    /**
     * @brief 线程局部变量，指向当前线程的调度器主协程。
     * @details 每个线程都有自己独立的 t_scheduler_fiber 变量，初始值为 nullptr。
     *          当线程关联到调度器后，该变量会被设置为调度器的主协程指针。
     */
    static thread_local Fiber*t_scheduler_fiber=nullptr;

    Scheduler::Scheduler(size_t threads,bool use_caller,const std::string&name)
            :m_name(name){
            SYLAR_ASSERT(threads>0);

            if(use_caller){//是否使用当前线程
                Sylar::Fiber::GetThis();//确保当前线程初始化了协程环境
                --threads;

                SYLAR_ASSERT(GetThis()==nullptr);
                t_scheduler=this;//将当前调度器实例赋值给线程局部变量

                //创建住协程m_rootFiber,其入口函数为Scheduler::run
                m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));
                //设置当前线程的名称为调度器名称
                Sylar::Thread::SetName(m_name);

                t_scheduler_fiber=m_rootFiber.get();
                m_rootThread=Sylar::GetThreadId();
                m_threadIds.push_back(m_rootThread);
            }else{
                m_rootThread=-1;
            }
            m_threadCount=threads;
    }

    Scheduler::~Scheduler(){
        SYLAR_ASSERT(m_stopping);
        if(GetThis()==this){
            t_scheduler=nullptr;
        }
    }

    Scheduler*Scheduler::GetThis(){
        return t_scheduler;
    }

    Fiber*Scheduler::GetMainFiber(){
        return t_scheduler_fiber;
    }

    void Scheduler::start(){
        //加锁保护调度器的状态，防止多线程调用start导致竞态条件
        MutexType::Lock lock(m_mutex);
        if(!m_stopping){
            return ;
        }
        //设置调度器状态为运行中
        m_stopping=false;
        SYLAR_ASSERT(m_threads.empty());
        //创建线程池
        m_threads.resize(m_threadCount);
        for(size_t i=0;i<m_threadCount;++i){
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this)
            ,m_name+"_"+std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();
    }

    void Scheduler::stop(){
        m_autoStop=true;
        //检查主协程状态，如果主协程已完成且没有线程，直接退出
        if(m_rootFiber
                    &&m_threadCount==0
                    &&(m_rootFiber->getState()==Fiber::TERM
                      ||m_rootFiber->getState()==Fiber::INIT)){
            SYLAR_LOG_INFO(g_logger)<<this<<" stopped";
            m_stopping=true;

            if(stopping()){
                return;
            }
        }
        //检查当前线程是否属于调度器
        if(m_rootThread!=-1){
            SYLAR_ASSERT(GetThis()==this);//当前线程是调度器的主线程
        }else{
            SYLAR_ASSERT(GetThis()!=this);//当前线程未绑定当前调度器
        }
        //设置停止标志并通知线程
        m_stopping=true;
        for(size_t i=0;i<m_threadCount;++i){
            tickle();
        }
        if(m_rootFiber){
            tickle();
        }
        //如果存在主协程，且调度器未完全停止，调用主协程的call方法执行剩余任务
        if(m_rootFiber){
            if(!stopping()){
                m_rootFiber->call();
            }
        }
        std::vector<Thread::ptr>thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for(auto& i:thrs){
            i->join();
        }
    }

    void Scheduler::setThis(){
        t_scheduler=this;
    }

    void Scheduler::run() {
     //打印调试日志，标记当前线程进入调度器运行状态   
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    //启用hook功能，将阻塞的调用替换为非阻塞版本
    set_hook_enable(true);
    //将当前调度器实例绑定到线程局部变量t_scheduler
    setThis();
    //如果当前线程不是主线程，将当前线程的主协程指针保存到t_scheduler_fiber
    if(Sylar::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    //空闲协程，用于在没有任务可执行时防止线程空转
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;

    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果任务绑定了特定线程且当前线程不匹配，跳过该任务
                if(it->thread != -1 && it->thread != Sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                //如果任务是协程且状态位EXEC,跳过该任务
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                //如果找到了可执行任务，将其从队列中移除，并标记为活跃任务
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me) {
            tickle();
        }
        //如果任务是协程，调用其swapIn()方法切换到协程执行
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;
            //如果协程状态位Ready,重新加入任务队列
            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb) {
            //如果任务是回调函数，将其包装为协程后执行
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            //如果没有活跃任务，切换到空闲协程执行
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        Sylar::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this) {
        if(thread == -1 || thread == Sylar::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    m_caller = Scheduler::GetThis();
    if(target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if(m_caller) {
        m_caller->switchTo();
    }
}

}