#include "Sylar/sylar.h"
#include "Sylar/rock/rock_stream.h"

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

Sylar::RockConnection::ptr conn(new Sylar::RockConnection);
void run() {
    conn->setAutoConnect(true);
    Sylar::Address::ptr addr = Sylar::Address::LookupAny("127.0.0.1:8061");
    if(!conn->connect(addr)) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " false";
    }
    conn->start();

    Sylar::IOManager::GetThis()->addTimer(1000, [](){
        Sylar::RockRequest::ptr req(new Sylar::RockRequest);
        static uint32_t s_sn = 0;
        req->setSn(++s_sn);
        req->setCmd(100);
        req->setBody("hello world sn=" + std::to_string(s_sn));

        auto rsp = conn->request(req, 300);
        if(rsp->response) {
            SYLAR_LOG_INFO(g_logger) << rsp->response->toString();
        } else {
            SYLAR_LOG_INFO(g_logger) << "error result=" << rsp->result;
        }
    }, true);
}

int main(int argc, char** argv) {
    Sylar::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
