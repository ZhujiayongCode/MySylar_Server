/**
 * @file log.h
 * @brief 日志模块封装
 * @author Zhujiayong
 * @email zhujiayong_hhh@163.com
 * @date 2025-04-09
 * @copyright Copyright (c) 2025年 Zhujiayong All rights reserved
 */

#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        Sylar::LogEventWrap(Sylar::LogEvent::ptr(new Sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, Sylar::GetThreadId(),\
                Sylar::GetFiberId(), time(0), Sylar::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, Sylar::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, Sylar::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, Sylar::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, Sylar::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, Sylar::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        Sylar::LogEventWrap(Sylar::LogEvent::ptr(new Sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, Sylar::GetThreadId(),\
                Sylar::GetFiberId(), time(0), Sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, Sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, Sylar::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, Sylar::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, Sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, Sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define SYLAR_LOG_ROOT() Sylar::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define SYLAR_LOG_NAME(name) Sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace Sylar
{
    class Logger;
    class LoggerManager;

    /**@brief日志级别**/
    class LogLevel
    {
        public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL
        };
        // 将日志级别转换为字符串
        static const char* toString(LogLevel::Level level);
        // 将字符串转换为日志级别
        static LogLevel::Level fromString(const std::string& str);
    };

    /**@brief 日志事件 */
    class LogEvent
    {
        public:
        using ptr = std::shared_ptr<LogEvent>;
        /*
        * @brief 构造函数
        * @param[in] logger 日志器
        * @param[in] level 日志级别
        * @param[in] file 文件名
        * @param[in] line 行号
        * @param[in] elapse 程序启动到现在的毫秒数
        * @param[in] thread_id 线程id
        * @param[in] fiber_id 协程id
        * @param[in] time 时间
        * @param[in] thread_name 线程名称
        */
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, 
                const char* file, int32_t line, uint32_t elapse, 
                uint32_t thread_id, uint32_t fiber_id, uint64_t time, 
                const std::string& thread_name);

        /*返回日志器*/
        std::shared_ptr<Logger> getLogger() const { return m_logger; }

        /*返回日志级别*/
        LogLevel::Level getLevel() const { return m_level; }

        /*返回文件名*/
        const char* getFile() const { return m_file; }

        /*返回行号*/
        int32_t getLine() const { return m_line; }

        /*返回程序启动到现在的毫秒数*/
        uint32_t getElapse() const { return m_elapse; } 

        /*返回线程id*/
        uint32_t getThreadId() const { return m_threadId; }

        /*返回协程id*/
        uint32_t getFiberId() const { return m_fiberId; }
        
        /*返回时间*/
        uint64_t getTime() const { return m_time; }

        /*返回线程名称*/
        const std::string& getThreadName() const { return m_threadName; }   

        /*返回日志内容流*/
        std::stringstream& getSS() { return m_ss; }

        /*返回日志内容*/
        std::string getContent() const { return m_ss.str(); }

        /*格式化写入日志内容*/
        void format(const char* fmt, ...);

        /*格式化写入日志内容*/
        void format(const char* fmt, va_list al);

        private:
        std::shared_ptr<Logger> m_logger;//日志器

        LogLevel::Level m_level;//日志级别

        const char* m_file;//文件名

        int32_t m_line;//行号

        uint32_t m_elapse;//程序启动到现在的毫秒数

        uint32_t m_threadId;//线程id

        uint32_t m_fiberId;//协程id

        uint64_t m_time;//时间

        std::string m_threadName;//线程名称

        std::stringstream m_ss;//日志内容流
    };

    /**@brief 日志事件包装器*/
    class LogEventWrap
    {
        public:
        LogEventWrap(LogEvent::ptr event);

        ~LogEventWrap();
        /*获取日志事件*/
        LogEvent::ptr getEvent() const { return m_event; }

        /*获取日志内容流*/
        std::stringstream& getSS() { return m_event->getSS(); }

        private:
        LogEvent::ptr m_event;
    };
   
    /*日志格式化*/
    class LogFormatter
    {
     public:
        using ptr = std::shared_ptr<LogFormatter>;
         /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
        LogFormatter(const std::string& pattern);

        /**
     * @brief 返回格式化日志文本
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
        std::string format(LogEvent::ptr event);

        /*返回格式化日志文本*/
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        /*返回格式化日志文本*/
        std::ostream&format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    public:
        /*日志内容项格式化*/
        class FormatItem{
        public:
            using ptr=std::shared_ptr<FormatItem>;
            virtual ~FormatItem();
            /**
             * @brierf 格式化日志到流
             * @parm[in,out] os 日志输出流
             * @parm[in] Logger 日志器
             * @parm[in] level 日志级别
             * @parm[in] event 日志事件*/
            virtual void format(std::ostream &os,std::shared_ptr<Logger>Logger,LogLevel::Level level,LogEvent::ptr event)=0;
        };
        /*brief 初始化，解析日志模板*/
        void init();
        
        bool isError()const{return this->m_error;};

        const std::string getPattern()const{return this->m_pattern;};
        
    private:
        std::string m_pattern;//日志格式模板

        std::vector<FormatItem::ptr>m_items;//日志格式解析后格式

        bool m_error=false;//是否有错误
    }; 
    /*brief日志输出目标*/
    class LogAppender{
        friend class Logger;
    public:
        using ptr=std::shared_ptr<LogAppender>;
        using MutexType=Spinlock;
        
        virtual ~LogAppender(){};
        /** 
        *@brief 写入日志
        * @parm[in] Logger 日志器
        * @parm[in] Lever 日志级别
        * @parm[in] event 日志事件
        */
       virtual void log(std::shared_ptr<Logger>Logger,LogLevel::Level level,LogEvent::ptr event)=0;
       
       /*@brief 将日志输出目标的配置转成YAML String*/
       virtual std::string toYamlString()=0;

       /*@brief 更改日志格式器*/
       void setFormatter(LogFormatter::ptr val);

       /*@brief 获取日志格式器*/
       LogFormatter::ptr getFormatter();

       /*@brief 获取日志级别*/
       LogLevel::Level getLevel()const{return m_level;};

       /*@brief 获取日志级别*/
       void setLevel(LogLevel::Level val){m_level=val;};
    protected:
       LogLevel::Level m_level=LogLevel::DEBUG;
       //是否拥有自己的日志格式器
       bool m_hasFormatter=false;
       //mutex
       MutexType m_mutex;
       //日志格式器
       LogFormatter::ptr m_formatter;
    };

    /*brief 日志器*/
    class Logger:public std::enable_shared_from_this<Logger>{
        friend class LoggerManager;
    public:
        using ptr=std::shared_ptr<Logger>;
        using MutexType=Spinlock;

        Logger(const std::string&name="root");
        
        void log(LogLevel::Level level,LogEvent::ptr event);
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();

        LogLevel::Level getLevel()const{return m_level;};
        void setLevel(LogLevel::Level val){m_level=val;};

        const std::string&getName()const{return m_name;};

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string&val);
        LogFormatter::ptr getFormatter();

        std::string toYamlString();//将日志器的配置转为YAML String
    private:
        std::string m_name;
        LogLevel::Level m_level;
        MutexType m_mutex;
        std::list<LogAppender::ptr>m_appenders;//目标日志集合
        LogFormatter::ptr m_formatter;//日志格式器
        Logger::ptr m_root;//主日志器
    };

    /*@brief 输出到控制台的Appender*/
    class StdoutLogAppender:public LogAppender{
        using ptr=std::shared_ptr<StdoutLogAppender>;
        void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event)override;
        std::string toYamlString()override;
    };

    /*brief 输出到文件的Appender*/
    class FileLogAppender:public LogAppender{
    public:
        using ptr=std::shared_ptr<FileLogAppender>;
        FileLogAppender(const std::string&filename);
        void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event)override;
        std::string toYamlString()override;

        bool reopen();//重新打开日志文件，成功返回true
    private:
        std::string m_filename;//文件路径
        std::ofstream m_filestream;//文件流
        uint64_t m_lastTime=0;//上次重新打开时间
    };

    /*brief 日志器管理类*/
    class  LoggerManager{
    public:
        using MutexType=Spinlock;

        LoggerManager();

        Logger::ptr getLogger(const std::string&name);
        void init();
        Logger::ptr getRoot()const{return m_root;};
        std::string toYamlString();
    private:
        MutexType m_mutex;
        std::map<std::string,Logger::ptr>m_loggers;//日志器容器
        Logger::ptr m_root;//主日志器
    };

    //日志器管理类单例模式
    using LoggerMgr=Sylar::Singleton<LoggerManager>;
}

#endif