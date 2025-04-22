#ifndef __SYLAR_ZK_CLIENT_H__
#define __SYLAR_ZK_CLIENT_H__

#include <memory>
#include <functional>
#include <string>
#include <stdint.h>
#include <vector>

#ifndef THREADED
#define THREADED
#endif

#include <zookeeper/zookeeper.h>

namespace Sylar {

/**
 * @class ZKClient
 * @brief 封装 ZooKeeper 客户端功能的类，提供与 ZooKeeper 服务交互的接口。
 * 
 * 该类继承自 std::enable_shared_from_this，允许在类内部安全地获取指向自身的 shared_ptr。
 */
class ZKClient : public std::enable_shared_from_this<ZKClient> {
public:
    /**
     * @class EventType
     * @brief 定义 ZooKeeper 客户端事件类型的常量类。
     * 
     * 这些常量对应 ZooKeeper 库中的事件类型，用于标识不同的事件。
     */
    class EventType {
    public:
        static const int CREATED; // = ZOO_CREATED_EVENT;
        static const int DELETED; // = ZOO_DELETED_EVENT;
        static const int CHANGED; // = ZOO_CHANGED_EVENT;
        // 子节点变更事件，对应 ZOO_CHILD_EVENT
        static const int CHILD  ; 
        // 会话状态变更事件，对应 ZOO_SESSION_EVENT
        static const int SESSION; 
        // 取消监听事件，对应 ZOO_NOTWATCHING_EVENT
        static const int NOWATCHING; 
    };

    /**
     * @class FlagsType
     * @brief 定义 ZooKeeper 节点创建标志的常量类。
     * 
     * 这些常量对应 ZooKeeper 库中的节点创建标志，用于指定节点的属性。
     */
    class FlagsType {
    public:
        static const int EPHEMERAL; // = ZOO_EPHEMERAL;
        static const int SEQUENCE;  //  = ZOO_SEQUENCE;
        static const int CONTAINER; // = ZOO_CONTAINER;
    };

    /**
     * @class StateType
     * @brief 定义 ZooKeeper 客户端会话状态的常量类。
     * 
     * 这些常量对应 ZooKeeper 库中的会话状态，用于标识客户端与 ZooKeeper 服务的连接状态。
     */
    class StateType {
    public:
        static const int EXPIRED_SESSION; // = ZOO_EXPIRED_SESSION_STATE;
        static const int AUTH_FAILED; // = ZOO_AUTH_FAILED_STATE;
        static const int CONNECTING; // = ZOO_CONNECTING_STATE;
        static const int ASSOCIATING; // = ZOO_ASSOCIATING_STATE;
        static const int CONNECTED; // = ZOO_CONNECTED_STATE;
        static const int READONLY; // = ZOO_READONLY_STATE;
        static const int NOTCONNECTED; // = ZOO_NOTCONNECTED_STATE;
    };

    typedef std::shared_ptr<ZKClient> ptr;
    // 定义监听器回调函数类型，用于处理 ZooKeeper 事件
    typedef std::function<void(int type, int stat, const std::string& path, ZKClient::ptr)> watcher_callback;
    // 定义日志回调函数类型，用于记录日志信息
    typedef void(*log_callback)(const char *message);

    ZKClient();
    ~ZKClient();

    /**
     * @brief 初始化 ZooKeeper 客户端。
     * 
     * @param hosts ZooKeeper 服务器地址列表，格式为 "host1:port1,host2:port2,..."。
     * @param recv_timeout 接收超时时间，单位为毫秒。
     * @param cb 监听器回调函数，用于处理 ZooKeeper 事件。
     * @param lcb 日志回调函数，用于记录日志信息，默认为 nullptr。
     * @return 初始化成功返回 true，失败返回 false。
     */
    bool init(const std::string& hosts, int recv_timeout, watcher_callback cb, log_callback lcb = nullptr);

    /**
     * @brief 设置 ZooKeeper 服务器地址列表。
     * 
     * @param hosts ZooKeeper 服务器地址列表，格式为 "host1:port1,host2:port2,..."。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t setServers(const std::string& hosts);

    /**
     * @brief 创建一个 ZooKeeper 节点。
     * 
     * @param path 节点路径。
     * @param val 节点存储的数据。
     * @param new_path 输出参数，返回实际创建的节点路径（如果是顺序节点）。
     * @param acl 节点的访问控制列表，默认为 ZOO_OPEN_ACL_UNSAFE。
     * @param flags 节点创建标志，默认为 0。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t create(const std::string& path, const std::string& val, std::string& new_path
                   , const struct ACL_vector* acl = &ZOO_OPEN_ACL_UNSAFE
                   , int flags = 0);

    /**
     * @brief 检查指定路径的节点是否存在。
     * 
     * @param path 节点路径。
     * @param watch 是否设置监听。
     * @param stat 输出参数，返回节点的状态信息，默认为 nullptr。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t exists(const std::string& path, bool watch, Stat* stat = nullptr);

    /**
     * @brief 删除指定路径的节点。
     * 
     * @param path 节点路径。
     * @param version 节点版本号，默认为 -1，表示不检查版本。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t del(const std::string& path, int version = -1);

    /**
     * @brief 获取指定路径节点的数据。
     * 
     * @param path 节点路径。
     * @param val 输出参数，返回节点存储的数据。
     * @param watch 是否设置监听。
     * @param stat 输出参数，返回节点的状态信息，默认为 nullptr。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t get(const std::string& path, std::string& val, bool watch, Stat* stat = nullptr);

    /**
     * @brief 获取 ZooKeeper 配置信息。
     * 
     * @param val 输出参数，返回 ZooKeeper 配置信息。
     * @param watch 是否设置监听。
     * @param stat 输出参数，返回配置节点的状态信息，默认为 nullptr。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t getConfig(std::string& val, bool watch, Stat* stat = nullptr);

    /**
     * @brief 设置指定路径节点的数据。
     * 
     * @param path 节点路径。
     * @param val 要设置的节点数据。
     * @param version 节点版本号，默认为 -1，表示不检查版本。
     * @param stat 输出参数，返回节点的状态信息，默认为 nullptr。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t set(const std::string& path, const std::string& val, int version = -1, Stat* stat = nullptr);

    /**
     * @brief 获取指定路径节点的所有子节点名称。
     * 
     * @param path 节点路径。
     * @param val 输出参数，返回子节点名称列表。
     * @param watch 是否设置监听。
     * @param stat 输出参数，返回节点的状态信息，默认为 nullptr。
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t getChildren(const std::string& path, std::vector<std::string>& val, bool watch, Stat* stat = nullptr);

    /**
     * @brief 关闭 ZooKeeper 客户端连接。
     * 
     * @return 操作结果，返回值含义参考 ZooKeeper 库文档。
     */
    int32_t close();

    /**
     * @brief 获取当前 ZooKeeper 客户端的会话状态。
     * 
     * @return 当前会话状态，对应 StateType 中的常量。
     */
    int32_t getState();

    /**
     * @brief 获取当前连接的 ZooKeeper 服务器地址。
     * 
     * @return 当前连接的服务器地址。
     */
    std::string  getCurrentServer();

    /**
     * @brief 重新连接到 ZooKeeper 服务器。
     * 
     * @return 重新连接成功返回 true，失败返回 false。
     */
    bool reconnect();
private:
    /**
     * @brief ZooKeeper 事件监听器的静态回调函数。
     * 
     * @param zh ZooKeeper 客户端句柄。
     * @param type 事件类型，对应 EventType 中的常量。
     * @param stat 会话状态，对应 StateType 中的常量。
     * @param path 事件关联的节点路径。
     * @param watcherCtx 监听器上下文指针。
     */
    static void OnWatcher(zhandle_t *zh, int type, int stat, const char *path,void *watcherCtx);

    // 定义内部使用的监听器回调函数类型
    typedef std::function<void(int type, int stat, const std::string& path)> watcher_callback2;
private:
    // ZooKeeper 客户端句柄
    zhandle_t* m_handle;
    // ZooKeeper 服务器地址列表
    std::string m_hosts;
    // 内部使用的监听器回调函数
    watcher_callback2 m_watcherCb;
    // 日志回调函数
    log_callback m_logCb;
    // 接收超时时间，单位为毫秒
    int32_t m_recvTimeout;
};

}

#endif
