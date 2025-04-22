#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"

Sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace Sylar {

static Sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    Sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

/**
 * @brief 初始化钩子函数，获取原始系统调用函数的地址
 * 
 * 该函数会检查钩子是否已经初始化过，如果已经初始化则直接返回，
 * 避免重复初始化。使用宏展开的方式动态加载一系列系统调用函数的原始地址。
 */
void hook_init() {
    // 静态布尔变量，用于标记钩子函数是否已经初始化，避免重复初始化
    static bool is_inited = false;
    // 如果已经初始化过，直接返回
    if(is_inited) {
        return;
    }
    // 定义宏 XX，用于将系统调用函数名拼接成对应的函数指针变量名，
    // 并使用 dlsym 函数从动态链接库中获取该函数的原始地址
    // name ## _f 是拼接后的函数指针变量名
    // (name ## _fun) 是将函数名拼接成对应的函数指针类型
    // dlsym(RTLD_NEXT, #name) 从当前库之后的库中查找名为 name 的符号地址
    #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    // 展开 HOOK_FUN 宏，将 XX 宏应用到一系列系统调用函数名上，获取它们的原始地址
    HOOK_FUN(XX);
    // 取消 XX 宏的定义，避免后续代码误用
    #undef XX

    // 标记钩子函数已经初始化
    is_inited = true;
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
/**
 * @brief 处理带有钩子功能的 I/O 操作
 * 
 * 该函数用于实现对系统 I/O 调用的钩子处理，支持超时机制和协程调度。
 * 当钩子功能启用时，会检查文件描述符的状态，若操作暂时不可用（EAGAIN），
 * 则会添加事件监听和定时器，让当前协程让出执行权，等待操作可继续执行。
 * 
 * @tparam OriginFun 原始系统调用函数的类型
 * @tparam Args 可变参数模板，用于传递原始系统调用所需的参数
 * @param fd 文件描述符，代表要进行 I/O 操作的文件或套接字
 * @param fun 原始系统调用函数的指针
 * @param hook_fun_name 钩子函数的名称，用于日志输出
 * @param event 要监听的 I/O 事件，如 Sylar::IOManager::READ 或 Sylar::IOManager::WRITE
 * @param timeout_so 超时选项，如 SO_RCVTIMEO 或 SO_SNDTIMEO
 * @param args 传递给原始系统调用函数的参数
 * @return ssize_t I/O 操作的返回值，-1 表示出错
 */
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    if(!Sylar::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    // 如果文件描述符不是套接字，或者用户设置了非阻塞模式，直接调用原始系统调用函数
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    // 定义重试标签，当 I/O 操作因 EAGAIN 错误暂停后，可跳转至此重新尝试
retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 如果 I/O 操作被信号中断（errno == EINTR），则重试操作
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 如果 I/O 操作返回 -1 且错误码为 EAGAIN，说明资源暂时不可用
    if(n == -1 && errno == EAGAIN) {
        Sylar::IOManager* iom = Sylar::IOManager::GetThis();
        Sylar::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if(to != (uint64_t)-1) {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (Sylar::IOManager::Event)(event));
            }, winfo);
        }

        int rt = iom->addEvent(fd, (Sylar::IOManager::Event)(event));
        if(SYLAR_UNLIKELY(rt)) {
            SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            // 当前协程让出执行权，等待事件触发或超时
            Sylar::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            // 跳转至 retry 标签，重新尝试 I/O 操作
            goto retry;
        }
    }
    
    // 如果 I/O 操作没有遇到 EAGAIN 错误，直接返回操作结果
    return n;
}


extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!Sylar::t_hook_enable) {
        return sleep_f(seconds);
    }

    Sylar::Fiber::ptr fiber = Sylar::Fiber::GetThis();
    Sylar::IOManager* iom = Sylar::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(Sylar::Scheduler::*)
            (Sylar::Fiber::ptr, int thread))&Sylar::IOManager::schedule
            ,iom, fiber, -1));
    Sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!Sylar::t_hook_enable) {
        return usleep_f(usec);
    }
    Sylar::Fiber::ptr fiber = Sylar::Fiber::GetThis();
    Sylar::IOManager* iom = Sylar::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(Sylar::Scheduler::*)
            (Sylar::Fiber::ptr, int thread))&Sylar::IOManager::schedule
            ,iom, fiber, -1));
    Sylar::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!Sylar::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    Sylar::Fiber::ptr fiber = Sylar::Fiber::GetThis();
    Sylar::IOManager* iom = Sylar::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(Sylar::Scheduler::*)
            (Sylar::Fiber::ptr, int thread))&Sylar::IOManager::schedule
            ,iom, fiber, -1));
    Sylar::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!Sylar::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    Sylar::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!Sylar::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if(n == 0) {
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }

    Sylar::IOManager* iom = Sylar::IOManager::GetThis();
    Sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, Sylar::IOManager::WRITE);
        }, winfo);
    }

    int rt = iom->addEvent(fd, Sylar::IOManager::WRITE);
    if(rt == 0) {
        Sylar::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if(timer) {
            timer->cancel();
        }
        SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, Sylar::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", Sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        Sylar::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", Sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", Sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", Sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", Sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", Sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", Sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", Sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", Sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", Sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", Sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!Sylar::t_hook_enable) {
        return close_f(fd);
    }

    Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = Sylar::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        Sylar::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

/**
 * @brief 对 fcntl 系统调用进行钩子处理的函数
 * 
 * 该函数根据不同的 fcntl 命令（cmd）对文件描述符进行操作，
 * 同时处理套接字文件描述符的用户非阻塞模式设置和获取，
 * 其他命令则直接调用原始的 fcntl 函数。
 * 
 * @param fd 文件描述符
 * @param cmd fcntl 命令
 * @param ... 可变参数，根据不同的 cmd 有不同的含义
 * @return int 操作结果，与原始 fcntl 函数返回值一致
 */
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    // 初始化可变参数列表，从 cmd 之后开始获取参数
    va_start(va, cmd);
    // 根据不同的 fcntl 命令进行不同的处理
    switch(cmd) {
        // 设置文件状态标志
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                // 根据传入的参数设置用户非阻塞模式
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        // 获取文件状态标志
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!Sylar::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            Sylar::FdCtx::ptr ctx = Sylar::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
