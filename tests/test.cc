#include <iostream>
#include "Sylar/log.h"
#include "Sylar/util.h"

int main(int argc, char** argv) {
    Sylar::Logger::ptr logger(new Sylar::Logger);
    logger->addAppender(Sylar::LogAppender::ptr(new Sylar::StdoutLogAppender));

    Sylar::FileLogAppender::ptr file_appender(new Sylar::FileLogAppender("./log.txt"));
    Sylar::LogFormatter::ptr fmt(new Sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(Sylar::LogLevel::ERROR);

    logger->addAppender(file_appender);

    //Sylar::LogEvent::ptr event(new Sylar::LogEvent(__FILE__, __LINE__, 0, Sylar::GetThreadId(), Sylar::GetFiberId(), time(0)));
    //event->getSS() << "hello Sylar log";
    //logger->log(Sylar::LogLevel::DEBUG, event);
    std::cout << "hello Sylar log" << std::endl;

    SYLAR_LOG_INFO(logger) << "test macro";
    SYLAR_LOG_ERROR(logger) << "test macro error";

    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = Sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";
    return 0;
}
