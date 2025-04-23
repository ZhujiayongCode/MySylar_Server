#include "Sylar/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

struct A {
    A() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for(size_t i = 0; i < content.size(); ++i) {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

A a;

int main(int argc, char** argv) {
    std::cout << "argc=" << argc << std::endl;
    Sylar::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    Sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    Sylar::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!Sylar::EnvMgr::GetInstance()->init(argc, argv)) {
        Sylar::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << Sylar::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << Sylar::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << Sylar::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << Sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << Sylar::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << Sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if(Sylar::EnvMgr::GetInstance()->has("p")) {
        Sylar::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}
