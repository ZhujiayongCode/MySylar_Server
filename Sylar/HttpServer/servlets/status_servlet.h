#ifndef __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__

#include "Sylar/HttpServer/servlet.h"

namespace Sylar {
namespace http {

class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(Sylar::http::HttpRequest::ptr request
                   , Sylar::http::HttpResponse::ptr response
                   , Sylar::http::HttpSession::ptr session) override;
};

}
}

#endif
