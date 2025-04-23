#include "Sylar/module.h"
#include "Sylar/singleton.h"
#include <iostream>
#include "Sylar/log.h"
#include "Sylar/DataBase/redis.h"

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class A {
public:
    A() {
        std::cout << "A::A " << this << std::endl;
    }

    ~A() {
        std::cout << "A::~A " << this << std::endl;
    }

};

class MyModule : public Sylar::RockModule {
public:
    MyModule()
        :RockModule("hello", "1.0", "") {
        //Sylar::Singleton<A>::GetInstance();
    }

    bool onLoad() override {
        Sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onLoad------------" << std::endl;
        return true;
    }

    bool onUnload() override {
        Sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onUnload------------" << std::endl;
        return true;
    }

    bool onServerReady() {
        registerService("rock", "Sylar.top", "blog");
        auto rpy = Sylar::RedisUtil::Cmd("local", "get abc");
        if(!rpy) {
            SYLAR_LOG_ERROR(g_logger) << "redis cmd get abc error";
        } else {
            SYLAR_LOG_ERROR(g_logger) << "redis get abc: "
                << (rpy->str ? rpy->str : "(null)");
        }
        return true;
    }

    bool handleRockRequest(Sylar::RockRequest::ptr request
                        ,Sylar::RockResponse::ptr response
                        ,Sylar::RockStream::ptr stream) {
        //SYLAR_LOG_INFO(g_logger) << "handleRockRequest " << request->toString();
        //sleep(1);
        response->setResult(0);
        response->setResultStr("ok");
        response->setBody("echo: " + request->getBody());

        usleep(100 * 1000);
        auto addr = stream->getLocalAddressString();
        if(addr.find("8061") != std::string::npos) {
            if(rand() % 100 < 50) {
                usleep(10 * 1000);
            } else if(rand() % 100 < 10) {
                response->setResult(-1000);
            }
        } else {
            //if(rand() % 100 < 25) {
            //    usleep(10 * 1000);
            //} else if(rand() % 100 < 10) {
            //    response->setResult(-1000);
            //}
        }
        return true;
        //return rand() % 100 < 90;
    }

    bool handleRockNotify(Sylar::RockNotify::ptr notify 
                        ,Sylar::RockStream::ptr stream) {
        SYLAR_LOG_INFO(g_logger) << "handleRockNotify " << notify->toString();
        return true;
    }

};

extern "C" {

Sylar::Module* CreateModule() {
    Sylar::Singleton<A>::GetInstance();
    std::cout << "=============CreateModule=================" << std::endl;
    return new MyModule;
}

void DestoryModule(Sylar::Module* ptr) {
    std::cout << "=============DestoryModule=================" << std::endl;
    delete ptr;
}

}
