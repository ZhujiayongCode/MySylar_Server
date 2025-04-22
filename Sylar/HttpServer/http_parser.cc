#include "http_parser.h"
#include "Sylar/log.h"
#include "Sylar/config.h"
#include <string.h>

namespace Sylar {
namespace http {

static Sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/**
 * @brief 查找并获取 http 请求缓冲区大小的配置变量
 * 若配置文件中未设置该值，则使用默认值 4KB
 */
static Sylar::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    Sylar::Config::Lookup("http.request.buffer_size"
                ,(uint64_t)(4 * 1024), "http request buffer size");

/**
 * @brief 查找并获取 http 请求最大正文大小的配置变量
 * 若配置文件中未设置该值，则使用默认值 64MB
 */
static Sylar::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    Sylar::Config::Lookup("http.request.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http request max body size");

/**
 * @brief 查找并获取 http 响应缓冲区大小的配置变量
 * 若配置文件中未设置该值，则使用默认值 4KB
 */
static Sylar::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    Sylar::Config::Lookup("http.response.buffer_size"
                ,(uint64_t)(4 * 1024), "http response buffer size");

/**
 * @brief 查找并获取 http 响应最大正文大小的配置变量
 * 若配置文件中未设置该值，则使用默认值 64MB
 */
static Sylar::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
    Sylar::Config::Lookup("http.response.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}


namespace {
/**
 * @brief 初始化 http 请求和响应的缓冲区大小及最大正文大小，并设置配置变更监听器
 */
struct _RequestSizeIniter {
    _RequestSizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        g_http_request_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_buffer_size = nv;
        });

        g_http_request_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_max_body_size = nv;
        });

        g_http_response_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_buffer_size = nv;
        });

        g_http_response_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_max_body_size = nv;
        });
    }
};
// 静态对象，确保在程序启动时执行初始化操作
static _RequestSizeIniter _init;
}

/**
 * @brief 处理 http 请求方法的回调函数
 * @param data 指向 HttpRequestParser 对象的指针
 * @param at 方法字符串的起始地址
 * @param length 方法字符串的长度
 */
void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = CharsToHttpMethod(at);

    if(m == HttpMethod::INVALID_METHOD) {
        SYLAR_LOG_WARN(g_logger) << "invalid http request method: "
            << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length) {
}

void on_request_fragment(void *data, const char *at, size_t length) {
    //SYLAR_LOG_INFO(g_logger) << "on_request_fragment:" << std::string(at, length);
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

/**
 * @brief 处理 http 请求版本的回调函数
 * @param data 指向 HttpRequestParser 对象的指针
 * @param at 版本字符串的起始地址
 * @param length 版本字符串的长度
 */
void on_request_version(void *data, const char *at, size_t length) {
    // 将 data 转换为 HttpRequestParser 指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    // 检查是否为 HTTP/1.1
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;

    } 
    // 检查是否为 HTTP/1.0
    else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        SYLAR_LOG_WARN(g_logger) << "invalid http request version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {
    //HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
}

/**
 * @brief 处理 http 请求头字段的回调函数
 * @param data 指向 HttpRequestParser 对象的指针
 * @param field 字段名的起始地址
 * @param flen 字段名的长度
 * @param value 字段值的起始地址
 * @param vlen 字段值的长度
 */
void on_request_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0) {
        SYLAR_LOG_WARN(g_logger) << "invalid http request field length == 0";
        //parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

/**
 * @brief HttpRequestParser 构造函数
 * 初始化请求对象，解析器，并设置回调函数
 */
HttpRequestParser::HttpRequestParser()
    :m_error(0) {
    m_data.reset(new Sylar::http::HttpRequest);
    http_parser_init(&m_parser);
    // 设置解析器的回调函数
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    // 设置解析器的上下文数据
    m_parser.data = this;
}

/**
 * @brief 获取 http 请求的内容长度
 * @return http 请求的内容长度，若未设置则返回 0
 */
uint64_t HttpRequestParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}




/**
 * @brief 执行 http 请求解析
 * @param data 待解析的数据
 * @param len 待解析数据的长度
 * @return 已处理的字节数
 * 1: 成功
 * -1: 有错误
 * >0: 已处理的字节数，且 data 有效数据为 len - v;
 */
size_t HttpRequestParser::execute(char* data, size_t len) {
    // 执行解析操作
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    // 移动未处理的数据
    memmove(data, data + offset, (len - offset));
    return offset;
}

/**
 * @brief 检查 http 请求解析是否完成
 * @return 若解析完成返回非零值，否则返回 0
 */
int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}

/**
 * @brief 检查 http 请求解析是否有错误
 * @return 若有错误返回非零值，否则返回 0
 */
int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}

/**
 * @brief 处理 http 响应原因短语的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 原因短语字符串的起始地址
 * @param length 原因短语字符串的长度
 */
void on_response_reason(void *data, const char *at, size_t length) {
    // 将 data 转换为 HttpResponseParser 指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    // 设置响应对象的原因短语
    parser->getData()->setReason(std::string(at, length));
}

/**
 * @brief 处理 http 响应状态码的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 状态码字符串的起始地址
 * @param length 状态码字符串的长度
 */
void on_response_status(void *data, const char *at, size_t length) {
    // 将 data 转换为 HttpResponseParser 指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    // 将字符串转换为 HttpStatus 枚举值
    HttpStatus status = (HttpStatus)(atoi(at));
    // 设置响应对象的状态码
    parser->getData()->setStatus(status);
}

/**
 * @brief 处理 http 响应分块大小的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 分块大小字符串的起始地址
 * @param length 分块大小字符串的长度
 */
void on_response_chunk(void *data, const char *at, size_t length) {
}

/**
 * @brief 处理 http 响应版本的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 版本字符串的起始地址
 * @param length 版本字符串的长度
 */
void on_response_version(void *data, const char *at, size_t length) {
    // 将 data 转换为 HttpResponseParser 指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    // 检查是否为 HTTP/1.1
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;

    } 
    // 检查是否为 HTTP/1.0
    else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        // 记录警告日志
        SYLAR_LOG_WARN(g_logger) << "invalid http response version: "
            << std::string(at, length);
        // 设置解析错误码
        parser->setError(1001);
        return;
    }

    // 设置响应对象的版本
    parser->getData()->setVersion(v);
}

/**
 * @brief 处理 http 响应头完成的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 字符串的起始地址
 * @param length 字符串的长度
 */
void on_response_header_done(void *data, const char *at, size_t length) {
}

/**
 * @brief 处理 http 响应最后一个分块的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param at 字符串的起始地址
 * @param length 字符串的长度
 */
void on_response_last_chunk(void *data, const char *at, size_t length) {
}

/**
 * @brief 处理 http 响应头字段的回调函数
 * @param data 指向 HttpResponseParser 对象的指针
 * @param field 字段名的起始地址
 * @param flen 字段名的长度
 * @param value 字段值的起始地址
 * @param vlen 字段值的长度
 */
void on_response_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    // 将 data 转换为 HttpResponseParser 指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    // 检查字段名长度是否为 0
    if(flen == 0) {
        // 记录警告日志
        SYLAR_LOG_WARN(g_logger) << "invalid http response field length == 0";
        //parser->setError(1002);
        return;
    }
    // 设置响应对象的头字段
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

/**
 * @brief HttpResponseParser 构造函数
 * 初始化响应对象，解析器，并设置回调函数
 */
HttpResponseParser::HttpResponseParser()
    :m_error(0) {
    m_data.reset(new Sylar::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

size_t HttpResponseParser::execute(char* data, size_t len, bool chunck) {
    if(chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);

    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}
}
