/**
 * @file service_discovery.h
 * @brief 服务发现模块的头文件，定义了服务发现相关的类和接口。
 * @details 该模块提供了服务注册、查询和监听服务变化的功能，支持基于 ZooKeeper 的实现。
 */

#ifndef __SYLAR_STREAMS_SERVICE_DISCOVERY_H__
#define __SYLAR_STREAMS_SERVICE_DISCOVERY_H__

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "Sylar/mutex.h"
#include "Sylar/iomanager.h"
#include "Sylar/zk_client.h"

namespace Sylar {

/**
 * @class ServiceItemInfo
 * @brief 表示一个服务项的信息，包含服务的 ID、IP、端口和附加数据。
 */
class ServiceItemInfo {
public:
    typedef std::shared_ptr<ServiceItemInfo> ptr;
    static ServiceItemInfo::ptr Create(const std::string& ip_and_port, const std::string& data);

    uint64_t getId() const { return m_id;}
    uint16_t getPort() const { return m_port;}
    const std::string& getIp() const { return m_ip;}
    const std::string& getData() const { return m_data;}

    std::string toString() const;
private:
    uint64_t m_id;       ///< 服务项的唯一 ID。
    uint16_t m_port;     ///< 服务项的端口。
    std::string m_ip;    ///< 服务项的 IP 地址。
    std::string m_data;  ///< 服务项的附加数据。
};

/**
 * @class IServiceDiscovery
 * @brief 服务发现接口类，定义了服务注册、查询和监听服务变化的基本方法。
 */
class IServiceDiscovery {
public:
    typedef std::shared_ptr<IServiceDiscovery> ptr;

    /**
     * @brief 服务变化回调函数类型。
     * @param domain 服务所属的域名。
     * @param service 服务的名称。
     * @param old_value 服务变化前的信息。
     * @param new_value 服务变化后的信息。
     */
    typedef std::function<void(const std::string& domain, const std::string& service
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value)> service_callback;
    virtual ~IServiceDiscovery() { }

    void registerServer(const std::string& domain, const std::string& service,
                        const std::string& ip_and_port, const std::string& data);
    void queryServer(const std::string& domain, const std::string& service);
    void listServer(std::unordered_map<std::string, std::unordered_map<std::string
                    ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > >& infos);
    void listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string
                            ,std::unordered_map<std::string, std::string> > >& infos);
    void listQueryServer(std::unordered_map<std::string, std::unordered_set<std::string> >& infos);

    virtual void start() = 0;
    virtual void stop() = 0;

    service_callback getServiceCallback() const { return m_cb;}

    /**
     * @brief 设置服务变化回调函数。
     * @param v 新的服务变化回调函数。
     */
    void setServiceCallback(service_callback v) { m_cb = v;}

    void setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v);
protected:
    Sylar::RWMutex m_mutex;
    //domain -> [service -> [id -> ServiceItemInfo] ]
    std::unordered_map<std::string, std::unordered_map<std::string
        ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > > m_datas;
    //domain -> [service -> [ip_and_port -> data] ]
    std::unordered_map<std::string, std::unordered_map<std::string
        ,std::unordered_map<std::string, std::string> > > m_registerInfos;
    //domain -> [service]
    std::unordered_map<std::string, std::unordered_set<std::string> > m_queryInfos;

    service_callback m_cb;
};

/**
 * @class ZKServiceDiscovery
 * @brief 基于 ZooKeeper 的服务发现实现类，继承自 IServiceDiscovery 接口。
 */
class ZKServiceDiscovery : public IServiceDiscovery
                          ,public std::enable_shared_from_this<ZKServiceDiscovery> {
public:
    typedef std::shared_ptr<ZKServiceDiscovery> ptr;
    ZKServiceDiscovery(const std::string& hosts);
    const std::string& getSelfInfo() const { return m_selfInfo;}
    void setSelfInfo(const std::string& v) { m_selfInfo = v;}
    const std::string& getSelfData() const { return m_selfData;}
    void setSelfData(const std::string& v) { m_selfData = v;}

    virtual void start();
    virtual void stop();
private:
    void onWatch(int type, int stat, const std::string& path, ZKClient::ptr);
    void onZKConnect(const std::string& path, ZKClient::ptr client);
    void onZKChild(const std::string& path, ZKClient::ptr client);
    void onZKChanged(const std::string& path, ZKClient::ptr client);
    void onZKDeleted(const std::string& path, ZKClient::ptr client);
    void onZKExpiredSession(const std::string& path, ZKClient::ptr client);

    bool registerInfo(const std::string& domain, const std::string& service, 
                      const std::string& ip_and_port, const std::string& data);
    bool queryInfo(const std::string& domain, const std::string& service);
    bool queryData(const std::string& domain, const std::string& service);

    bool existsOrCreate(const std::string& path);
    bool getChildren(const std::string& path);
private:
    std::string m_hosts;        ///< ZooKeeper 服务器的地址列表。
    std::string m_selfInfo;     ///< 自身的服务信息，格式为 "ip:port"。
    std::string m_selfData;     ///< 自身的服务附加数据。
    ZKClient::ptr m_client;     ///< 指向 ZooKeeper 客户端的智能指针。
    Sylar::Timer::ptr m_timer;  ///< 定时器指针。
    bool m_isOnTimer = false;   ///< 定时器是否正在运行的标志。
};

}

#endif
