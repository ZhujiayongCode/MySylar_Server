/**
 * @file protocol.h
 * @brief 自定义协议
 * @author Sylar.yin
 * @email 564628276@qq.com
 * @date 2019-07-03
 * @copyright Copyright (c) 2019年 Sylar.yin All rights reserved (www.Sylar.top)
 */
#ifndef __SYLAR_PROTOCOL_H__
#define __SYLAR_PROTOCOL_H__

#include <memory>
#include "Sylar/stream.h"
#include "Sylar/bytearray.h"

namespace Sylar {

/**
 * @class Message
 * @brief 消息类的基类，定义了消息的基本接口。
 * @details 所有具体的消息类型都应继承自该类，并实现其纯虚函数。
 */
class Message {
public:
    typedef std::shared_ptr<Message> ptr;
    enum MessageType {
        REQUEST = 1,  ///< 请求消息类型
        RESPONSE = 2, ///< 响应消息类型
        NOTIFY = 3    ///< 通知消息类型
    };
    virtual ~Message() {}

    /**
     * @brief 将消息转换为字节数组。
     * @return 指向字节数组的智能指针。
     */
    virtual ByteArray::ptr toByteArray();

    /**
     * @brief 将消息序列化到字节数组中。
     * @param bytearray 指向字节数组的智能指针。
     * @return 如果序列化成功返回 true，否则返回 false。
     */
    virtual bool serializeToByteArray(ByteArray::ptr bytearray) = 0;

    /**
     * @brief 从字节数组中解析消息。
     * @param bytearray 指向字节数组的智能指针。
     * @return 如果解析成功返回 true，否则返回 false。
     */
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) = 0;

    virtual std::string toString() const = 0;
    virtual const std::string& getName() const = 0;
    virtual int32_t getType() const = 0;
};

/**
 * @class MessageDecoder
 * @brief 消息解码器类，定义了解码和编码消息的接口。
 * @details 用于从流中解析消息，以及将消息序列化到流中。
 */
class MessageDecoder {
public:
    typedef std::shared_ptr<MessageDecoder> ptr;

    virtual ~MessageDecoder() {}
    virtual Message::ptr parseFrom(Stream::ptr stream) = 0;
    virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) = 0;
};

class Request : public Message {
public:
    typedef std::shared_ptr<Request> ptr;

    Request();

    uint32_t getSn() const { return m_sn;}
    uint32_t getCmd() const { return m_cmd;}

    void setSn(uint32_t v) { m_sn = v;}
    void setCmd(uint32_t v) { m_cmd = v;}

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
protected:
    uint32_t m_sn;  ///< 请求的序列号
    uint32_t m_cmd; ///< 请求的命令号
};

class Response : public Message {
public:
    typedef std::shared_ptr<Response> ptr;

    Response();

    uint32_t getSn() const { return m_sn;}
    uint32_t getCmd() const { return m_cmd;}
    uint32_t getResult() const { return m_result;}
    const std::string& getResultStr() const { return m_resultStr;}

    void setSn(uint32_t v) { m_sn = v;}
    void setCmd(uint32_t v) { m_cmd = v;}
    void setResult(uint32_t v) { m_result = v;}
    void setResultStr(const std::string& v) { m_resultStr = v;}

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
protected:
    uint32_t m_sn;       ///< 响应的序列号
    uint32_t m_cmd;      ///< 响应的命令号
    uint32_t m_result;   ///< 响应的结果码
    std::string m_resultStr; ///< 响应的结果字符串
};

class Notify : public Message {
public:
    typedef std::shared_ptr<Notify> ptr;
    Notify();

    uint32_t getNotify() const { return m_notify;}
    void setNotify(uint32_t v) { m_notify = v;}

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
protected:
    uint32_t m_notify; ///< 通知的通知号
};

}

#endif
