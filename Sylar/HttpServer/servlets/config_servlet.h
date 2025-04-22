/**
 * @file config_servlet.h
 * @brief 定义配置Servlet类的头文件
 * @details 该文件定义了一个用于处理配置相关HTTP请求的Servlet类。
 */

#ifndef __SYLAR_HTTP_SERVLETS_CONFIG_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_CONFIG_SERVLET_H__

#include "Sylar/HttpServer/servlet.h"

namespace Sylar {
namespace http {

/**
 * @class ConfigServlet
 * @brief 处理配置相关HTTP请求的Servlet类
 * @details 继承自Servlet基类，重写了handle方法以处理特定的HTTP请求。
 */
class ConfigServlet : public Servlet {
public:
    ConfigServlet();

    /**
     * @brief 处理HTTP请求的方法
     * @param request HTTP请求对象的智能指针
     * @param response HTTP响应对象的智能指针
     * @param session HTTP会话对象的智能指针
     * @return 处理结果的状态码
     * @details 重写基类的handle方法，用于处理配置相关的HTTP请求。
     */
    virtual int32_t handle(Sylar::http::HttpRequest::ptr request
                   , Sylar::http::HttpResponse::ptr response
                   , Sylar::http::HttpSession::ptr session) override;
};

}
}

#endif
