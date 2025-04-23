#include "Sylar/tcp_server.h"
#include "Sylar/iomanager.h"
#include "Sylar/log.h"

Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    auto addr = Sylar::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = Sylar::UnixAddress::ptr(new Sylar::UnixAddress("/tmp/unix_addr"));
    std::vector<Sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    Sylar::TcpServer::ptr tcp_server(new Sylar::TcpServer);
    std::vector<Sylar::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    Sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
