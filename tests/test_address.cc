#include "Sylar/address.h"
#include "Sylar/log.h"

Sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    std::vector<Sylar::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger) << "begin";
    bool v = Sylar::Address::Lookup(addrs, "localhost:3080");
    //bool v = Sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //bool v = Sylar::Address::Lookup(addrs, "www.Sylar.top", AF_INET);
    SYLAR_LOG_INFO(g_logger) << "end";
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = Sylar::Address::LookupAny("localhost:4080");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << *addr;
    } else {
        SYLAR_LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<Sylar::Address::ptr, uint32_t> > results;

    bool v = Sylar::Address::GetInterfaceAddresses(results);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        SYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = Sylar::IPAddress::Create("www.Sylar.top");
    auto addr = Sylar::IPAddress::Create("127.0.0.8");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    return 0;
}
