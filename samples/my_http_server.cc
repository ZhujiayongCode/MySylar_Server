#include "Sylar/HttpServer/http_server.h"
#include "Sylar/log.h"

Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
Sylar::IOManager::ptr worker;
void run() {
    g_logger->setLevel(Sylar::LogLevel::INFO);
    Sylar::Address::ptr addr = Sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    Sylar::http::HttpServer::ptr http_server(new Sylar::http::HttpServer(true, worker.get()));
    //Sylar::http::HttpServer::ptr http_server(new Sylar::http::HttpServer(true));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
        //http_server->loadCertificates("/home/apps/soft/Sylar/keys/server.crt", "/home/apps/soft/Sylar/keys/server.key");
    }

    http_server->start();
}

int main(int argc, char** argv) {
    Sylar::IOManager iom(1);
    worker.reset(new Sylar::IOManager(4, false));
    iom.schedule(run);
    return 0;
}
