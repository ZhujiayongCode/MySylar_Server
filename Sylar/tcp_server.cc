#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace Sylar {

static Sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

static Sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

TcpServer::TcpServer(Sylar::IOManager* worker,
                    Sylar::IOManager* io_worker,
                    Sylar::IOManager* accept_worker)
    :m_worker(worker)
    ,m_ioWorker(io_worker)
    ,m_acceptWorker(accept_worker)
    ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
    ,m_name("Sylar/1.0.0")
    ,m_isStop(true) {
}

TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

void TcpServer::setConf(const TcpServerConf& v) {
    m_conf.reset(new TcpServerConf(v));
}

bool TcpServer::bind(Sylar::Address::ptr addr, bool ssl) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}

/**
 * @brief 将 TCP 服务器绑定到多个地址，并启动监听
 * 
 * 该函数尝试将服务器绑定到给定的多个地址，并启动监听。如果绑定或监听失败，
 * 失败的地址将被添加到 fails 向量中。如果所有地址都绑定和监听成功，
 * 则返回 true；否则清空已绑定的套接字列表并返回 false。
 * 
 * @param addrs 要绑定的地址列表
 * @param fails 用于存储绑定或监听失败的地址
 * @param ssl 是否使用 SSL 加密
 * @return bool 如果所有地址都成功绑定并监听，返回 true；否则返回 false
 */
bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails
                        ,bool ssl) {
    // 设置服务器是否使用 SSL 加密
    m_ssl = ssl;
    // 遍历所有要绑定的地址
    for(auto& addr : addrs) {
        // 根据是否使用 SSL 选择创建普通 TCP 套接字或 SSL TCP 套接字
        Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        // 若绑定成功，尝试启动套接字监听
        if(!sock->listen()) {
            SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        // 若绑定和监听都成功，将该套接字添加到服务器的套接字列表中
        m_socks.push_back(sock);
    }

    if(!fails.empty()) {
        // 若有失败地址，清空已绑定的套接字列表
        m_socks.clear();
        return false;
    }

    // 若所有地址都成功绑定和监听，记录成功信息
    for(auto& i : m_socks) {
        SYLAR_LOG_INFO(g_logger) << "type=" << m_type
            << " name=" << m_name
            << " ssl=" << m_ssl
            << " server bind success: " << *i;
    }
    return true;
}

/**
 * @brief 开始接受客户端连接
 * 
 * 该函数在一个循环中持续尝试从指定的监听套接字接受客户端连接。
 * 只要服务器未停止，就会不断检查是否有新的客户端连接请求。
 * 当接收到新的客户端连接时，会设置客户端套接字的接收超时时间，
 * 并将处理客户端连接的任务调度到 I/O 工作线程中执行。
 * 
 * @param sock 用于监听客户端连接的套接字指针
 */
void TcpServer::startAccept(Socket::ptr sock) {
    // 只要服务器未停止，就持续接受客户端连接
    while(!m_isStop) {
        // 尝试从监听套接字接受一个新的客户端连接
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            // 将处理客户端连接的任务调度到 I/O 工作线程中执行
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                        shared_from_this(), client));
        } else {
            SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        // 为每个监听套接字调度一个任务，调用 startAccept 函数开始接受客户端连接
        // 使用 std::bind 绑定成员函数和参数，shared_from_this() 用于获取当前对象的智能指针
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;
}

/**
 * @brief 为服务器的所有 SSL 套接字加载证书和私钥文件
 * 
 * 该函数遍历服务器的所有套接字，尝试将每个套接字转换为 SSL 套接字。
 * 对于成功转换的 SSL 套接字，调用其 `loadCertificates` 方法加载指定的证书和私钥文件。
 * 如果任何一个 SSL 套接字加载证书和私钥失败，则整个函数返回 false；
 * 如果所有 SSL 套接字都成功加载证书和私钥，则返回 true。
 * 
 * @param cert_file 证书文件的路径
 * @param key_file 私钥文件的路径
 * @return bool 若所有 SSL 套接字都成功加载证书和私钥，返回 true；否则返回 false
 */
bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    for(auto& i : m_socks) {
        // 尝试将当前套接字转换为 SSL 套接字
        auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
        if(ssl_socket) {
            if(!ssl_socket->loadCertificates(cert_file, key_file)) {
                return false;
            }
        }
    }
    // 所有 SSL 套接字都成功加载证书和私钥，返回 true
    return true;
}

std::string TcpServer::toString(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name << " ssl=" << m_ssl
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

}
