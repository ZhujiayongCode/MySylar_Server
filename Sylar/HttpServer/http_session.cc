#include "http_session.h"
#include "http_parser.h"

namespace Sylar {
namespace http {

/**
 * @brief HttpSession 类的构造函数
 * 
 * 用于初始化 HttpSession 对象，调用 SocketStream 的构造函数来初始化底层的套接字流。
 * 
 * @param sock 指向 Socket 对象的智能指针，代表与客户端通信的套接字
 * @param owner 布尔值，指示是否拥有该套接字的所有权
 */
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

/**
 * @brief 从套接字接收并解析 HTTP 请求
 * 
 * 该函数尝试从套接字读取数据，并使用 HttpRequestParser 解析数据以生成 HttpRequest 对象。
 * 如果在读取或解析过程中发生错误，将关闭套接字并返回 nullptr。
 * 
 * @return 指向 HttpRequest 对象的智能指针，如果成功接收并解析请求；否则返回 nullptr
 */
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    // 获取缓冲区的原始指针
    char* data = buffer.get();
    // 已读取数据的偏移量
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        // 计算未解析的数据偏移量
        offset = len - nparse;
        if(offset == (int)buff_size) {
            // 缓冲区已满且仍未解析完成，关闭套接字并返回 nullptr
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    int64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    parser->getData()->init();
    return parser->getData();
}

/**
 * @brief 向客户端发送 HTTP 响应
 * 
 * 该函数将 HttpResponse 对象序列化为字符串，并将其发送到客户端。
 * 
 * @param rsp 指向 HttpResponse 对象的智能指针，代表要发送的 HTTP 响应
 * @return 成功发送的字节数，如果发生错误则返回负数
 */
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}
