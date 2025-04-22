/**
 * @file iomanager.h
 * @brief 基于epoll的IO协程调度器
 * @author Zhujiayong
 * @email zhujiayong_hhh@163.com
 * @copyright Copyright (c) 2025年 Zhujiayong All rights reserved 
 */
#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include"scheduler.h"
#include"timer.h"

namespace Sylar{

    /**
     * @brief 基于epoll的IO协程调度器
     */
    class IOManager:public Scheduler,public TimerManager{
        public:
        using ptr=std::shared_ptr<IOManager>;
        using RWMutexType=RWMutex;
        
        /**
         * @brief IO事件
         */
        enum Event{
            //无事件
            NONE=0x0,
            //读事件（EPOLLIN)
            READ=0x1,
            //写事件(EPOLLOUT)
            WRITE=0x4
        };
        private:
        /**
         * @brief Socket事件上下文类
         */
        struct FdContext{
            using MutexType=Mutex;

            struct EventContext{
                //事件执行的调度器
                Scheduler*scheduler=nullptr;
                //事件协程
                Fiber::ptr fiber;
                //事件的回调函数
                std::function<void()>cb;
            };

            /**
             * @brief 获取事件的上下文类
             * @brief[in] event 事件类型
             * @return 返回对应事件的上下文
             */
            EventContext&getContext(Event event);
            
            /**
             * @brief 重置事件上下文
             * @param[in,out] ctx 待重置的上下文类
             */
            void resetContext(EventContext&ctx);

            /**
             * @brief 触发事件
             * @param[in] event 事件类型
             */
            void triggerEvent(Event event);

            //读事件上下文
            EventContext read;
            //写事件上下文
            EventContext write;
            //事件关联的句柄
            int fd=0;
            //当前事件
            Event events=NONE;
            //事件的Mutex
            MutexType mutex;
        };

        public:
        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否将调用线程包含进去
         * @param[in] name 调度器的名称
         */
        IOManager(size_t threads=1,bool use_caller=true,const std::string&name="");

        ~IOManager();

        /**
         * @brif 添加事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @param[in] cb 事件回调函数
         * @return 添加成功返回0，失败返回-1
         */
        int addEvent(int fd,Event event,std::function<void()>cb=nullptr);

        /**
         * @brief 删除事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @attention 不会触发事件
         */
        bool delEvent(int fd,Event event);
        
        /**
         * @brief 取消事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @attention 如果事件正在执行，会触发事件
         */
        
        bool cancelEvent(int fd,Event event);

        /**
         * @brief 取消所有事件
         * @param[in] fd socket句柄
         */
        bool cancelAll(int fd);

        /**
         * @brief 返回当前的IOManager
         */
        static IOManager*GetThis();
        protected:
        void tickle()override;
        bool stopping()override;
        void idle()override;
        void onTimerInsertedAtFront()override;

        /**
         * @brief 重置socket句柄上下文的容器大小
         * @param[in] size 容量大小
         */
        void contextResize(size_t size);

        /**
         * @brief 判断是否可以停止
         * @parm[in] timeout最近要触发的定时器时间间隔
         * @return 返回是否可以停止
         */
        bool stopping(uint64_t&timeout);
        private:
        //epoll文件句柄
        int m_epfd=0;
        //pipe文件句柄
        int m_tickleFds[2];
        //当前等待执行的事件数量
        std::atomic<size_t>m_pendingEventCount={0};
        //IOManger的Mutex
        RWMutexType m_mutex;
        //soecket事件上下文的容器
        std::vector<FdContext*>m_fdContexts;

    };
}
#endif