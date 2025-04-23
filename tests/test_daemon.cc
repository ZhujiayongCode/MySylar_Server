#include "Sylar/daemon.h"
#include "Sylar/iomanager.h"
#include "Sylar/log.h"

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

Sylar::Timer::ptr timer;
int server_main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << Sylar::ProcessInfoMgr::GetInstance()->toString();
    Sylar::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
            SYLAR_LOG_INFO(g_logger) << "onTimer";
            static int count = 0;
            if(++count > 10) {
                exit(1);
            }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return Sylar::start_daemon(argc, argv, server_main, argc != 1);
}
