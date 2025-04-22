/**
 * @file load_balance.h
 * @brief 负载均衡相关类的定义，提供多种负载均衡算法和服务发现集成功能。
 * @details 包含统计信息管理、负载均衡项管理、不同负载均衡算法实现以及与服务发现的集成。
 */

#ifndef __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__
#define __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__

#include "Sylar/streams/socket_stream.h"
#include "Sylar/mutex.h"
#include "Sylar/util.h"
#include "Sylar/streams/service_discovery.h"
#include <vector>
#include <unordered_map>

namespace Sylar {

class HolderStatsSet;

/**
 * @class HolderStats
 * @brief 用于统计负载均衡项的各种状态信息，如使用时间、总请求数等。
 * @details 提供原子操作来更新统计信息，确保线程安全。
 */
class HolderStats {
friend class HolderStatsSet;
public:
    uint32_t getUsedTime() const { return m_usedTime; }
    uint32_t getTotal() const { return m_total; }
    uint32_t getDoing() const { return m_doing; }
    uint32_t getTimeouts() const { return m_timeouts; }
    uint32_t getOks() const { return m_oks; }
    uint32_t getErrs() const { return m_errs; }

    uint32_t incUsedTime(uint32_t v) { return Sylar::Atomic::addFetch(m_usedTime ,v);}
    uint32_t incTotal(uint32_t v) { return Sylar::Atomic::addFetch(m_total, v);}
    uint32_t incDoing(uint32_t v) { return Sylar::Atomic::addFetch(m_doing, v);}
    uint32_t incTimeouts(uint32_t v) { return Sylar::Atomic::addFetch(m_timeouts, v);}
    uint32_t incOks(uint32_t v) { return Sylar::Atomic::addFetch(m_oks, v);}
    uint32_t incErrs(uint32_t v) { return Sylar::Atomic::addFetch(m_errs, v);}

    uint32_t decDoing(uint32_t v) { return Sylar::Atomic::subFetch(m_doing, v);}
    void clear();

    /**
     * @brief 计算负载均衡项的权重
     * @param rate 权重计算的比率，默认为 1.0f
     * @return 计算得到的权重值
     */
    float getWeight(float rate = 1.0f);

    std::string toString();
private:
    uint32_t m_usedTime = 0;  ///< 总使用时间
    uint32_t m_total = 0;     ///< 总请求数
    uint32_t m_doing = 0;     ///< 正在处理的请求数
    uint32_t m_timeouts = 0;  ///< 超时请求数
    uint32_t m_oks = 0;       ///< 成功请求数
    uint32_t m_errs = 0;      ///< 错误请求数
};

/**
 * @class HolderStatsSet
 * @brief 管理多个 HolderStats 实例，提供统计信息的集合管理功能。
 * @details 可以根据时间获取不同的统计信息集合，并计算总体权重。
 */
class HolderStatsSet {
public:
    HolderStatsSet(uint32_t size = 5);
    HolderStats& get(const uint32_t& now = time(0));

    float getWeight(const uint32_t& now = time(0));

    HolderStats getTotal();
private:
    void init(const uint32_t& now);
private:
    uint32_t m_lastUpdateTime = 0; //seconds  ///< 最后更新时间，单位为秒
    std::vector<HolderStats> m_stats;  ///< 统计信息集合
};

/**
 * @class LoadBalanceItem
 * @brief 表示负载均衡中的一个项，包含套接字流、统计信息等。
 * @details 提供获取和设置套接字流、ID、权重等功能，以及判断是否有效和关闭的方法。
 */
class LoadBalanceItem {
public:
    typedef std::shared_ptr<LoadBalanceItem> ptr;
    virtual ~LoadBalanceItem() {}

    SocketStream::ptr getStream() const { return m_stream;}
    void setStream(SocketStream::ptr v) { m_stream = v;}

    void setId(uint64_t v) { m_id = v;}
    uint64_t getId() const { return m_id;}

    HolderStats& get(const uint32_t& now = time(0));

    template<class T>
    std::shared_ptr<T> getStreamAs() {
        return std::dynamic_pointer_cast<T>(m_stream);
    }

    virtual int32_t getWeight() { return m_weight;}
    void setWeight(int32_t v) { m_weight = v;}

    virtual bool isValid();
    void close();

    std::string toString();
protected:
    uint64_t m_id = 0;  ///< 负载均衡项的 ID
    SocketStream::ptr m_stream;  ///< 套接字流的智能指针
    int32_t m_weight = 0;  ///< 负载均衡项的权重
    HolderStatsSet m_stats;  ///< 统计信息集合
};

/**
 * @class ILoadBalance
 * @brief 负载均衡接口类，定义了获取负载均衡项的纯虚函数。
 * @details 包含负载均衡算法类型和错误码的枚举。
 */
class ILoadBalance {
public:
    enum Type {
        ROUNDROBIN = 1,  ///< 轮询算法
        WEIGHT = 2,      ///< 加权算法
        FAIR = 3         ///< 公平算法
    };

    enum Error {
        NO_SERVICE = -101,  ///< 没有可用服务
        NO_CONNECTION = -102  ///< 没有可用连接
    };
    typedef std::shared_ptr<ILoadBalance> ptr;
    virtual ~ILoadBalance() {}
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) = 0;
};

class LoadBalance : public ILoadBalance {
public:
    typedef Sylar::RWMutex RWMutexType;
    typedef std::shared_ptr<LoadBalance> ptr;
    void add(LoadBalanceItem::ptr v);
    void del(LoadBalanceItem::ptr v);
    void set(const std::vector<LoadBalanceItem::ptr>& vs);

    LoadBalanceItem::ptr getById(uint64_t id);
    void update(const std::unordered_map<uint64_t, LoadBalanceItem::ptr>& adds
                ,std::unordered_map<uint64_t, LoadBalanceItem::ptr>& dels);
    void init();

    std::string statusString(const std::string& prefix);
protected:
    virtual void initNolock() = 0;
    void checkInit();
protected:
    RWMutexType m_mutex;  ///< 读写锁，保证线程安全
    std::unordered_map<uint64_t, LoadBalanceItem::ptr> m_datas;  ///< 负载均衡项的映射
    uint64_t m_lastInitTime = 0;  ///< 最后初始化时间
};

/**
 * @class RoundRobinLoadBalance
 * @brief 轮询负载均衡器，继承自 LoadBalance 类。
 * @details 按照轮询的方式依次选择负载均衡项。
 */
class RoundRobinLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<RoundRobinLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;
protected:
    virtual void initNolock();
protected:
    std::vector<LoadBalanceItem::ptr> m_items;
};


/**
 * @class FairLoadBalanceItem
 * @brief 公平负载均衡项，继承自 LoadBalanceItem 类。
 * @details 提供清空统计信息和获取权重的方法。
 */
class FairLoadBalanceItem : public LoadBalanceItem {
//friend class FairLoadBalance;
public:
    typedef std::shared_ptr<FairLoadBalanceItem> ptr;

    void clear();
    virtual int32_t getWeight();
};

/**
 * @class WeightLoadBalance
 * @brief 加权负载均衡器，继承自 LoadBalance 类。
 * @details 根据负载均衡项的权重选择负载均衡项。
 */
class WeightLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<WeightLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;

    FairLoadBalanceItem::ptr getAsFair();
protected:
    virtual void initNolock();
private:
    int32_t getIdx(uint64_t v = -1);
protected:
    std::vector<LoadBalanceItem::ptr> m_items;
private:
    std::vector<int64_t> m_weights;
};



//class FairLoadBalance : public LoadBalance {
//public:
//    typedef std::shared_ptr<FairLoadBalance> ptr;
//    virtual LoadBalanceItem::ptr get() override;
//    FairLoadBalanceItem::ptr getAsFair();
//
//protected:
//    virtual void initNolock();
//private:
//    int32_t getIdx();
//protected:
//    std::vector<LoadBalanceItem::ptr> m_items;
//private:
//    std::vector<int32_t> m_weights;
//};

class SDLoadBalance {
public:
    typedef std::shared_ptr<SDLoadBalance> ptr;
    typedef std::function<SocketStream::ptr(ServiceItemInfo::ptr)> stream_callback;
    typedef Sylar::RWMutex RWMutexType;

    SDLoadBalance(IServiceDiscovery::ptr sd);
    virtual ~SDLoadBalance() {}

    virtual void start();
    virtual void stop();

    stream_callback getCb() const { return m_cb;}
    void setCb(stream_callback v) { m_cb = v;}

    /**
     * @brief 根据域名和服务名获取负载均衡器
     * @param domain 域名
     * @param service 服务名
     * @param auto_create 是否自动创建，默认为 false
     * @return 负载均衡器的智能指针，如果未找到且不自动创建则返回 nullptr
     */
    LoadBalance::ptr get(const std::string& domain, const std::string& service, bool auto_create = false);

    void initConf(const std::unordered_map<std::string, std::unordered_map<std::string, std::string> >& confs);

    std::string statusString();
private:
    void onServiceChange(const std::string& domain, const std::string& service
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value);

    /**
     * @brief 根据域名和服务名获取负载均衡算法类型
     * @param domain 域名
     * @param service 服务名
     * @return 负载均衡算法类型
     */
    ILoadBalance::Type getType(const std::string& domain, const std::string& service);
    LoadBalance::ptr createLoadBalance(ILoadBalance::Type type);
    LoadBalanceItem::ptr createLoadBalanceItem(ILoadBalance::Type type);
protected:
    RWMutexType m_mutex;  ///< 读写锁，保证线程安全
    IServiceDiscovery::ptr m_sd;  ///< 服务发现的智能指针
    //domain -> [ service -> [ LoadBalance ] ]
    std::unordered_map<std::string, std::unordered_map<std::string, LoadBalance::ptr> > m_datas;  ///< 负载均衡器的映射
    std::unordered_map<std::string, std::unordered_map<std::string, ILoadBalance::Type> > m_types;  ///< 负载均衡算法类型的映射
    ILoadBalance::Type m_defaultType = ILoadBalance::FAIR;  ///< 默认的负载均衡算法类型
    stream_callback m_cb;  ///< 套接字流创建回调函数
};

}

#endif
