/**
 * @file worker.h
 * @brief 实现任务管理与调度功能
 * @author Zhujiayong
 * @email zhujiayong_hhh@163.com
 * @date 2025-04-10
 * @copyright Copyright (c) 2025年 Zhujiayong All rights reserved
 */

#ifndef __SYLAR_WORKER_H__
#define __SYLAR_WORKER_H__

#include "mutex.h"
#include "singleton.h"
#include "log.h"
#include "iomanager.h"

namespace Sylar {


/**
 * @class WorkerGroup
 * @brief 工作线程组类，用于管理和调度一组任务。该类不可复制，并支持从自身获取智能指针。
 * @note 该类负责将任务按批次进行处理，并确保所有任务完成后才结束。
 */
class WorkerGroup : Noncopyable, public std::enable_shared_from_this<WorkerGroup> {
public:
    using ptr=std::shared_ptr<WorkerGroup>;
    /**
     * @brief 创建一个新的 WorkerGroup 实例
     * @param batch_size 每个批次处理的任务数量
     * @param s 指向调度器的指针，默认为当前调度器
     * @return 返回一个指向新创建的 WorkerGroup 实例的智能指针
     */
    static WorkerGroup::ptr Create(uint32_t batch_size, Sylar::Scheduler* s = Sylar::Scheduler::GetThis()) {
        return std::make_shared<WorkerGroup>(batch_size, s);
    }

    /**
     * @brief 构造函数
     * @param batch_size 每个批次处理的任务数量
     * @param s 指向调度器的指针，默认为当前调度器
     */
    WorkerGroup(uint32_t batch_size, Sylar::Scheduler* s = Sylar::Scheduler::GetThis());
    ~WorkerGroup();

    /**
     * @brief 调度一个任务进行处理
     * @param cb 要执行的任务，以 std::function<void()> 形式传入
     * @param thread 要执行任务的线程编号，-1 表示由调度器自动分配
     */
    void schedule(std::function<void()> cb, int thread = -1);

    /**
     * @brief 等待所有任务完成
     * @note 该方法会阻塞调用线程，直到所有任务都处理完毕
     */
    void waitAll();
private:
    /**
     * @brief 实际执行任务的方法
     * @param cb 要执行的任务，以 std::function<void()> 形式传入
     */
    void doWork(std::function<void()> cb);
private:
    /// 每个批次处理的任务数量
    uint32_t m_batchSize;
    /// 标记所有任务是否完成
    bool m_finish;
    /// 指向调度器的指针，用于任务调度
    Scheduler* m_scheduler;
    /// 协程信号量，用于任务同步
    FiberSemaphore m_sem;
};


/**
 * @class WorkerManager
 * @brief 工作调度器管理器类，用于管理多个调度器实例，提供添加、获取调度器以及任务调度等功能。
 */
class WorkerManager {
public:
    WorkerManager();

    /**
     * @brief 向管理器中添加一个调度器实例。
     * @param s 指向调度器的智能指针。
     */
    void add(Scheduler::ptr s);

    /**
     * @brief 根据名称获取对应的调度器实例。
     * @param name 调度器的名称。
     * @return 返回指向调度器的智能指针，如果未找到则返回空指针。
     */
    Scheduler::ptr get(const std::string& name);

    /**
     * @brief 根据名称获取对应的 IO 调度器实例。
     * @param name 调度器的名称。
     * @return 返回指向 IO 调度器的智能指针，如果未找到则返回空指针。
     */
    IOManager::ptr getAsIOManager(const std::string& name);

    /**
     * @brief 向指定名称的调度器中调度一个任务。
     * @tparam FiberOrCb 任务类型，可以是协程或者可调用对象。
     * @param name 调度器的名称。
     * @param fc 要调度的任务。
     * @param thread 要执行任务的线程编号，-1 表示由调度器自动分配。
     */
    template<class FiberOrCb>
    void schedule(const std::string& name, FiberOrCb fc, int thread = -1) {
        auto s = get(name);
        if(s) {
            s->schedule(fc, thread);
        } else {
            static Sylar::Logger::ptr s_logger = SYLAR_LOG_NAME("system");
            SYLAR_LOG_ERROR(s_logger) << "schedule name=" << name
                << " not exists";
        }
    }

    /**
     * @brief 向指定名称的调度器中批量调度任务。
     * @tparam Iter 迭代器类型，用于遍历任务集合。
     * @param name 调度器的名称。
     * @param begin 任务集合的起始迭代器。
     * @param end 任务集合的结束迭代器。
     */
    template<class Iter>
    void schedule(const std::string& name, Iter begin, Iter end) {
        auto s = get(name);
        if(s) {
            s->schedule(begin, end);
        } else {
            static Sylar::Logger::ptr s_logger = SYLAR_LOG_NAME("system");
            SYLAR_LOG_ERROR(s_logger) << "schedule name=" << name
                << " not exists";
        }
    }

    /**
     * @brief 初始化工作调度器管理器。
     * @return 初始化成功返回 true，失败返回 false。
     */
    bool init();

    /**
     * @brief 使用指定的配置信息初始化工作调度器管理器。
     * @param v 包含调度器配置信息的映射表。
     * @return 初始化成功返回 true，失败返回 false。
     */
    bool init(const std::map<std::string, std::map<std::string, std::string> >& v);

    /**
     * @brief 停止所有调度器。
     */
    void stop();

    /**
     * @brief 检查所有调度器是否已停止。
     * @return 若所有调度器已停止返回 true，否则返回 false。
     */
    bool isStoped() const { return m_stop;}

    /**
     * @brief 将工作调度器管理器的信息输出到指定的输出流。
     * @param os 输出流对象。
     * @return 返回输出流对象的引用。
     */
    std::ostream& dump(std::ostream& os);

    /**
     * @brief 获取管理器中调度器的数量。
     * @return 调度器的数量。
     */
    uint32_t getCount();
private:
    /// 存储调度器的映射表，键为调度器名称，值为调度器实例的向量
    std::map<std::string, std::vector<Scheduler::ptr> > m_datas;
    /// 标记所有调度器是否已停止
    bool m_stop;
};

/**
 * @brief 定义 WorkerManager 的单例类型，通过 WorkerMgr 可以方便地访问 WorkerManager 实例。
 */
typedef Sylar::Singleton<WorkerManager> WorkerMgr;

}

#endif
