/**
 * @file http_session.h
 * @brief HTTPSession封装
 * @author Sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-07
 * @copyright Copyright (c) 2019年 Sylar.yin All rights reserved (www.Sylar.top)
 */

#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "Sylar/streams/socket_stream.h"
#include "http.h"

namespace Sylar {
namespace http {

/**
 * @brief HTTPSession封装
 */
class HttpSession : public SocketStream {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP请求
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}
}

#endif
