#ifndef __SYLAR_NS_NS_CLIENT_H__
#define __SYLAR_NS_NS_CLIENT_H__

#include "Sylar/rock/rock_stream.h"
#include "ns_protocol.h"

namespace Sylar {
namespace ns {

class NSClient : public RockConnection {
public:
    typedef std::shared_ptr<NSClient> ptr;
    NSClient();
    ~NSClient();

    const std::set<std::string>& getQueryDomains();
    void setQueryDomains(const std::set<std::string>& v);

    void addQueryDomain(const std::string& domain);
    void delQueryDomain(const std::string& domain);

    bool hasQueryDomain(const std::string& domain);

    RockResult::ptr query();

    void init();
    void uninit();
    NSDomainSet::ptr getDomains() const { return m_domains;}
private:
    void onQueryDomainChange();
    bool onConnect(Sylar::AsyncSocketStream::ptr stream);
    void onDisconnect(Sylar::AsyncSocketStream::ptr stream);
    bool onNotify(Sylar::RockNotify::ptr ,Sylar::RockStream::ptr);

    void onTimer();
private:
    Sylar::RWMutex m_mutex;
    std::set<std::string> m_queryDomains;
    NSDomainSet::ptr m_domains;
    uint32_t m_sn = 0;
    Sylar::Timer::ptr m_timer;
};

}
}

#endif
