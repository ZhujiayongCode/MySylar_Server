#include "Sylar/HttpServer/http.h"
#include "Sylar/log.h"

void test_request() {
    Sylar::http::HttpRequest::ptr req(new Sylar::http::HttpRequest);
    req->setHeader("host" , "www.Sylar.top");
    req->setBody("hello Sylar");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    Sylar::http::HttpResponse::ptr rsp(new Sylar::http::HttpResponse);
    rsp->setHeader("X-X", "Sylar");
    rsp->setBody("hello Sylar");
    rsp->setStatus((Sylar::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
