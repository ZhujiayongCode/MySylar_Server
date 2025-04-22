#ifndef __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__
#define __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__

#include "socket_stream.h"
#include <list>
#include <unordered_map>
#include <boost/any.hpp>

namespace Sylar {

/**
 * @class AsyncSocketStream
 * @brief 异步套接字流类，继承自 SocketStream，支持异步操作
 * @details 该类提供了异步套接字流的基本功能，包括启动、关闭、发送和接收数据等，
 *          同时支持自动重连和回调函数机制。
 */
class AsyncSocketStream : public SocketStream
                         ,public std::enable_shared_from_this<AsyncSocketStream> {
public:
    typedef std::shared_ptr<AsyncSocketStream> ptr;
    typedef Sylar::RWMutex RWMutexType;
    typedef std::function<bool(AsyncSocketStream::ptr)> connect_callback;
    typedef std::function<void(AsyncSocketStream::ptr)> disconnect_callback;

    AsyncSocketStream(Socket::ptr sock, bool owner = true);

    virtual bool start();
    virtual void close() override;
public:
    enum Error {
        OK = 0,         ///< 操作成功
        TIMEOUT = -1,   ///< 操作超时
        IO_ERROR = -2,  ///< 输入输出错误
        NOT_CONNECT = -3 ///< 未连接
    };
protected:
    /**
     * @struct SendCtx
     * @brief 发送上下文的基类，用于封装发送操作的相关信息
     */
    struct SendCtx {
    public:
        typedef std::shared_ptr<SendCtx> ptr;
        virtual ~SendCtx() {}

        virtual bool doSend(AsyncSocketStream::ptr stream) = 0;
    };

    /**
     * @struct Ctx
     * @brief 上下文结构，继承自 SendCtx，用于管理异步操作的状态
     */
    struct Ctx : public SendCtx {
    public:
        typedef std::shared_ptr<Ctx> ptr;
        virtual ~Ctx() {}
        Ctx();

        uint32_t sn;        ///< 序列号
        uint32_t timeout;   ///< 超时时间
        uint32_t result;    ///< 操作结果
        bool timed;         ///< 是否超时

        Scheduler* scheduler; ///< 调度器指针
        Fiber::ptr fiber;     ///< 协程指针
        Timer::ptr timer;     ///< 定时器指针

        virtual void doRsp();
    };

public:
    void setWorker(Sylar::IOManager* v) { m_worker = v;}
    Sylar::IOManager* getWorker() const { return m_worker;}

    void setIOManager(Sylar::IOManager* v) { m_iomanager = v;}
    Sylar::IOManager* getIOManager() const { return m_iomanager;}

    bool isAutoConnect() const { return m_autoConnect;}
    void setAutoConnect(bool v) { m_autoConnect = v;}

    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v) { m_connectCb = v;}
    void setDisconnectCb(disconnect_callback v) { m_disconnectCb = v;}

    template<class T>
    void setData(const T& v) { m_data = v;}

    template<class T>
    T getData() const {
        try {
            return boost::any_cast<T>(m_data);
        } catch (...) {
        }
        return T();
    }
protected:
    virtual void doRead();
    virtual void doWrite();
    virtual void startRead();
    virtual void startWrite();
    virtual void onTimeOut(Ctx::ptr ctx);
    virtual Ctx::ptr doRecv() = 0;

    Ctx::ptr getCtx(uint32_t sn);
    Ctx::ptr getAndDelCtx(uint32_t sn);

    template<class T>
    std::shared_ptr<T> getCtxAs(uint32_t sn) {
        auto ctx = getCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    template<class T>
    std::shared_ptr<T> getAndDelCtxAs(uint32_t sn) {
        auto ctx = getAndDelCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    bool addCtx(Ctx::ptr ctx);
    bool enqueue(SendCtx::ptr ctx);

    bool innerClose();
    bool waitFiber();
protected:
    Sylar::FiberSemaphore m_sem;        ///< 协程信号量
    Sylar::FiberSemaphore m_waitSem;    ///< 等待协程信号量
    RWMutexType m_queueMutex;           ///< 队列互斥锁
    std::list<SendCtx::ptr> m_queue;    ///< 发送上下文队列
    RWMutexType m_mutex;                ///< 互斥锁
    std::unordered_map<uint32_t, Ctx::ptr> m_ctxs; ///< 上下文映射表

    uint32_t m_sn;                      ///< 序列号
    bool m_autoConnect;                 ///< 自动重连标志
    Sylar::Timer::ptr m_timer;          ///< 定时器指针
    Sylar::IOManager* m_iomanager;      ///< I/O 调度器指针
    Sylar::IOManager* m_worker;         ///< 工作调度器指针

    connect_callback m_connectCb;       ///< 连接成功回调函数
    disconnect_callback m_disconnectCb; ///< 断开连接回调函数

    boost::any m_data;                  ///< 通用数据
};

/**
 * @class AsyncSocketStreamManager
 * @brief 异步套接字流管理器类，用于管理多个异步套接字流
 */
class AsyncSocketStreamManager {
public:
    typedef Sylar::RWMutex RWMutexType;
    typedef AsyncSocketStream::connect_callback connect_callback;
    typedef AsyncSocketStream::disconnect_callback disconnect_callback;

    AsyncSocketStreamManager();
    virtual ~AsyncSocketStreamManager() {}

    void add(AsyncSocketStream::ptr stream);
    void clear();
    void setConnection(const std::vector<AsyncSocketStream::ptr>& streams);
    AsyncSocketStream::ptr get();
    template<class T>
    std::shared_ptr<T> getAs() {
        auto rt = get();
        if(rt) {
            return std::dynamic_pointer_cast<T>(rt);
        }
        return nullptr;
    }

    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v);
    void setDisconnectCb(disconnect_callback v);
private:
    RWMutexType m_mutex;                ///< 互斥锁
    uint32_t m_size;                    ///< 异步套接字流数量
    uint32_t m_idx;                     ///< 当前索引
    std::vector<AsyncSocketStream::ptr> m_datas; ///< 异步套接字流列表
    connect_callback m_connectCb;       ///< 连接成功回调函数
    disconnect_callback m_disconnectCb; ///< 断开连接回调函数
};

}

#endif
