#ifndef __SYLAR_APPLICATION_H__
#define __SYLAR_APPLICATION_H__

#include "Sylar/HttpServer/http_server.h"
#include "Sylar/streams/service_discovery.h"
#include "Sylar/rock/rock_stream.h"

namespace Sylar {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance;}
    bool init(int argc, char** argv);
    bool run();

    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);

    ZKServiceDiscovery::ptr getServiceDiscovery() const { return m_serviceDiscovery;}
    RockSDLoadBalance::ptr getRockSDLoadBalance() const { return m_rockSDLoadBalance;}
private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    //std::vector<Sylar::http::HttpServer::ptr> m_httpservers;
    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;

    ZKServiceDiscovery::ptr m_serviceDiscovery;
    RockSDLoadBalance::ptr m_rockSDLoadBalance;
};

}

#endif
