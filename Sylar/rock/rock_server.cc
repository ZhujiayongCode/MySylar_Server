#include "rock_server.h"
#include "Sylar/log.h"
#include "Sylar/module.h"

namespace Sylar {

static Sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

RockServer::RockServer(const std::string& type
                       ,Sylar::IOManager* worker
                       ,Sylar::IOManager* io_worker
                       ,Sylar::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_type = type;
}

void RockServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
    Sylar::RockSession::ptr session(new Sylar::RockSession(client));
    session->setWorker(m_worker);
    ModuleMgr::GetInstance()->foreach(Module::ROCK,
            [session](Module::ptr m) {
        m->onConnect(session);
    });
    session->setDisconnectCb(
        [](AsyncSocketStream::ptr stream) {
             ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [stream](Module::ptr m) {
                m->onDisconnect(stream);
            });
        }
    );
    session->setRequestHandler(
        [](Sylar::RockRequest::ptr req
           ,Sylar::RockResponse::ptr rsp
           ,Sylar::RockStream::ptr conn)->bool {
            //SYLAR_LOG_INFO(g_logger) << "handleReq " << req->toString()
            //                         << " body=" << req->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, req, rsp, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleRequest(req, rsp, conn);
            });
            return rt;
        }
    ); 
    session->setNotifyHandler(
        [](Sylar::RockNotify::ptr nty
           ,Sylar::RockStream::ptr conn)->bool {
            SYLAR_LOG_INFO(g_logger) << "handleNty " << nty->toString()
                                     << " body=" << nty->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, nty, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleNotify(nty, conn);
            });
            return rt;
        }
    );
    session->start();
}

}
