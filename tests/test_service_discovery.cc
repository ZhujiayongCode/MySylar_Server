#include "Sylar/streams/service_discovery.h"
#include "Sylar/iomanager.h"
#include "Sylar/rock/rock_stream.h"
#include "Sylar/log.h"
#include "Sylar/worker.h"

Sylar::ZKServiceDiscovery::ptr zksd(new Sylar::ZKServiceDiscovery("127.0.0.1:21812"));
Sylar::RockSDLoadBalance::ptr rsdlb(new Sylar::RockSDLoadBalance(zksd));

static Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

std::atomic<uint32_t> s_id;
void on_timer() {
    g_logger->setLevel(Sylar::LogLevel::INFO);
    //SYLAR_LOG_INFO(g_logger) << "on_timer";
    Sylar::RockRequest::ptr req(new Sylar::RockRequest);
    req->setSn(++s_id);
    req->setCmd(100);
    req->setBody("hello");

    auto rt = rsdlb->request("Sylar.top", "blog", req, 1000);
    if(!rt->response) {
        if(req->getSn() % 50 == 0) {
            SYLAR_LOG_ERROR(g_logger) << "invalid response: " << rt->toString();
        }
    } else {
        if(req->getSn() % 1000 == 0) {
            SYLAR_LOG_INFO(g_logger) << rt->toString();
        }
    }
}

void run() {
    zksd->setSelfInfo("127.0.0.1:2222");
    zksd->setSelfData("aaaa");

    std::unordered_map<std::string, std::unordered_map<std::string,std::string> > confs;
    confs["Sylar.top"]["blog"] = "fair";
    rsdlb->start(confs);
    //SYLAR_LOG_INFO(g_logger) << "on_timer---";

    Sylar::IOManager::GetThis()->addTimer(1, on_timer, true);
}

int main(int argc, char** argv) {
    Sylar::WorkerMgr::GetInstance()->init({
        {"service_io", {
            {"thread_num", "1"}
        }}
    });
    Sylar::IOManager iom(1);
    iom.addTimer(1000, [](){
            std::cout << rsdlb->statusString() << std::endl;
    }, true);
    iom.schedule(run);
    return 0;
}
