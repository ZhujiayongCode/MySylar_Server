#ifndef __SYLAR_ROCK_SERVER_H__
#define __SYLAR_ROCK_SERVER_H__

#include "Sylar/rock/rock_stream.h"
#include "Sylar/tcp_server.h"

namespace Sylar {

class RockServer : public TcpServer {
public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string& type = "rock"
               ,Sylar::IOManager* worker = Sylar::IOManager::GetThis()
               ,Sylar::IOManager* io_worker = Sylar::IOManager::GetThis()
               ,Sylar::IOManager* accept_worker = Sylar::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};

}

#endif
