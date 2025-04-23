#include "Sylar/HttpServer/ws_server.h"
#include "Sylar/log.h"

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    Sylar::http::WSServer::ptr server(new Sylar::http::WSServer);
    Sylar::Address::ptr addr = Sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](Sylar::http::HttpRequest::ptr header
                  ,Sylar::http::WSFrameMessage::ptr msg
                  ,Sylar::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/Sylar", fun);
    while(!server->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char** argv) {
    Sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
