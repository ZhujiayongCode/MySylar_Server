#include "worker.h"
#include "config.h"
#include "util.h"

namespace Sylar {

/**
 * @brief 定义一个全局配置变量指针，用于存储工作线程的配置信息。
 * 
 * 该配置变量通过 `Sylar::Config::Lookup` 方法从配置系统中查找名为 "workers" 的配置项。
 * 如果配置项不存在，则使用默认值 `std::map<std::string, std::map<std::string, std::string> >()` 初始化。
 * 该配置项主要用于存储工作线程的相关配置信息，注释说明该配置项的用途为 "worker config"。
 */
static Sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_worker_config
    = Sylar::Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string> >(), "worker config");

WorkerGroup::WorkerGroup(uint32_t batch_size, Sylar::Scheduler* s)
    :m_batchSize(batch_size)
    ,m_finish(false)
    ,m_scheduler(s)
    ,m_sem(batch_size) {
}

WorkerGroup::~WorkerGroup() {
    waitAll();
}

void WorkerGroup::schedule(std::function<void()> cb, int thread) {
    m_sem.wait();
    m_scheduler->schedule(std::bind(&WorkerGroup::doWork
                          ,shared_from_this(), cb), thread);
}

void WorkerGroup::doWork(std::function<void()> cb) {
    cb();
    m_sem.notify();
}

void WorkerGroup::waitAll() {
    if(!m_finish) {
        m_finish = true;
        for(uint32_t i = 0; i < m_batchSize; ++i) {
            m_sem.wait();
        }
    }
}

WorkerManager::WorkerManager()
    :m_stop(false) {
}

void WorkerManager::add(Scheduler::ptr s) {
    m_datas[s->getName()].push_back(s);
}

Scheduler::ptr WorkerManager::get(const std::string& name) {
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.size() == 1) {
        return it->second[0];
    }
    return it->second[rand() % it->second.size()];
}

IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string> >& v) {
    // 遍历配置映射表中的每个调度器配置项
    for(auto& i : v) {
        // 获取当前调度器的名称
        std::string name = i.first;
        // 从配置项中获取线程数量，若未指定则默认为 1
        int32_t thread_num = Sylar::GetParamValue(i.second, "thread_num", 1);
        // 从配置项中获取调度器实例数量，若未指定则默认为 1
        int32_t worker_num = Sylar::GetParamValue(i.second, "worker_num", 1);

        // 根据配置的实例数量创建调度器实例
        for(int32_t x = 0; x < worker_num; ++x) {
            Scheduler::ptr s;
            // 第一个调度器使用原始名称，后续调度器添加编号后缀
            if(!x) {
                s = std::make_shared<IOManager>(thread_num, false, name);
            } else {
                s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(x));
            }
            // 将创建好的调度器实例添加到管理器中
            add(s);
        }
    }
    // 如果没有添加任何调度器，则标记管理器已停止
    m_stop = m_datas.empty();
    return true;
}

bool WorkerManager::init() {
    auto workers = g_worker_config->getValue();
    return init(workers);
}

void WorkerManager::stop() {
    if(m_stop) {
        return;
    }
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->schedule([](){});
            n->stop();
        }
    }
    m_datas.clear();
    m_stop = true;
}

uint32_t WorkerManager::getCount() {
    return m_datas.size();
}

std::ostream& WorkerManager::dump(std::ostream& os) {
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->dump(os) << std::endl;
        }
    }
    return os;
}

}
