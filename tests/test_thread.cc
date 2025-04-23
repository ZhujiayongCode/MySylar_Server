#include "Sylar/sylar.h"
#include <unistd.h>

Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
//Sylar::RWMutex s_mutex;
Sylar::Mutex s_mutex;

void fun1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << Sylar::Thread::GetName()
                             << " this.name: " << Sylar::Thread::GetThis()->getName()
                             << " id: " << Sylar::GetThreadId()
                             << " this.id: " << Sylar::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        //Sylar::RWMutex::WriteLock lock(s_mutex);
        Sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/Sylar/test/Sylar/bin/conf/log2.yml");
    Sylar::Config::LoadFromYaml(root);

    std::vector<Sylar::Thread::ptr> thrs;
    for(int i = 0; i < 1; ++i) {
        Sylar::Thread::ptr thr(new Sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
        //Sylar::Thread::ptr thr2(new Sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        //thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
