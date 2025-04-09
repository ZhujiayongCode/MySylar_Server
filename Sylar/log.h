/**
 * @file log.h
 * @brief 日志模块封装
 * @author Zhujiayong
 * @email zhujiayong@163.com
 * @date 2025-04-09
 * @copyright Copyright (c) 2025年 Zhujiayong All rights reserved
 */

#ifndef MySylar_Server_LOG_H__
#define MySylar_Server_LOG_H__

#include <string>
#include <memory>
#include <sstream>

namespace Sylar
{
    class Logger;

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
        /*构造函数*/
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
        std::string format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    };
}
#endif