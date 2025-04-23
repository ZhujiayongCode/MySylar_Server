#include "Sylar/HttpServer/http_server.h"
#include "Sylar/log.h"

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

#define XX(...) #__VA_ARGS__


Sylar::IOManager::ptr worker;
void run() {
    g_logger->setLevel(Sylar::LogLevel::INFO);
    //Sylar::http::HttpServer::ptr server(new Sylar::http::HttpServer(true, worker.get(), Sylar::IOManager::GetThis()));
    Sylar::http::HttpServer::ptr server(new Sylar::http::HttpServer(true));
    Sylar::Address::ptr addr = Sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/Sylar/xx", [](Sylar::http::HttpRequest::ptr req
                ,Sylar::http::HttpResponse::ptr rsp
                ,Sylar::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/Sylar/*", [](Sylar::http::HttpRequest::ptr req
                ,Sylar::http::HttpResponse::ptr rsp
                ,Sylar::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/sylarx/*", [](Sylar::http::HttpRequest::ptr req
                ,Sylar::http::HttpResponse::ptr rsp
                ,Sylar::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    Sylar::IOManager iom(1, true, "main");
    worker.reset(new Sylar::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
