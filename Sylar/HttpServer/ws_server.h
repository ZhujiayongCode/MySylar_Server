#ifndef __SYLAR_HTTP_WS_SERVER_H__
#define __SYLAR_HTTP_WS_SERVER_H__

#include "Sylar/tcp_server.h"
#include "ws_session.h"
#include "ws_servlet.h"

namespace Sylar {
namespace http {

class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr;

    WSServer(Sylar::IOManager* worker = Sylar::IOManager::GetThis()
             , Sylar::IOManager* io_worker = Sylar::IOManager::GetThis()
             , Sylar::IOManager* accept_worker = Sylar::IOManager::GetThis());

    WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch;}
    void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
protected:
    WSServletDispatch::ptr m_dispatch;
};

}
}

#endif
