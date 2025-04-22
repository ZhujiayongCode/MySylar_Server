#ifndef __SYLAR_NS_NAME_SERVER_MODULE_H__
#define __SYLAR_NS_NAME_SERVER_MODULE_H__

#include "Sylar/module.h"
#include "ns_protocol.h"

namespace Sylar {
namespace ns {

class NameServerModule;
class NSClientInfo {
friend class NameServerModule;
public:
    typedef std::shared_ptr<NSClientInfo> ptr;
private:
    NSNode::ptr m_node;
    std::map<std::string, std::set<uint32_t> > m_domain2cmds;
};

class NameServerModule : public RockModule {
public:
    typedef std::shared_ptr<NameServerModule> ptr;
    NameServerModule();

    virtual bool handleRockRequest(Sylar::RockRequest::ptr request
                        ,Sylar::RockResponse::ptr response
                        ,Sylar::RockStream::ptr stream) override;
    virtual bool handleRockNotify(Sylar::RockNotify::ptr notify
                        ,Sylar::RockStream::ptr stream) override;
    virtual bool onConnect(Sylar::Stream::ptr stream) override;
    virtual bool onDisconnect(Sylar::Stream::ptr stream) override;
    virtual std::string statusString() override;
private:
    bool handleRegister(Sylar::RockRequest::ptr request
                        ,Sylar::RockResponse::ptr response
                        ,Sylar::RockStream::ptr stream);
    bool handleQuery(Sylar::RockRequest::ptr request
                        ,Sylar::RockResponse::ptr response
                        ,Sylar::RockStream::ptr stream);
    bool handleTick(Sylar::RockRequest::ptr request
                        ,Sylar::RockResponse::ptr response
                        ,Sylar::RockStream::ptr stream);

private:
    NSClientInfo::ptr get(Sylar::RockStream::ptr rs);
    void set(Sylar::RockStream::ptr rs, NSClientInfo::ptr info);

    void setQueryDomain(Sylar::RockStream::ptr rs, const std::set<std::string>& ds);

    void doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty);

    std::set<Sylar::RockStream::ptr> getStreams(const std::string& domain);
private:
    NSDomainSet::ptr m_domains;

    Sylar::RWMutex m_mutex;
    std::map<Sylar::RockStream::ptr, NSClientInfo::ptr> m_sessions;

    /// sessoin 关注的域名
    std::map<Sylar::RockStream::ptr, std::set<std::string> > m_queryDomains;
    /// 域名对应关注的session
    std::map<std::string, std::set<Sylar::RockStream::ptr> > m_domainToSessions;
};

}
}

#endif
