/**
 * @file http11_common.h
 * @brief 该文件包含HTTP/1.1相关的通用定义
 * 
 * 此文件使用头文件保护机制防止重复包含，同时定义了一些回调函数类型。
 */

/**
 * @brief 头文件保护宏，防止 http11_common.h 被重复包含
 * 
 * 如果 _http11_common_h 未被定义，则执行下面的代码并定义该宏。
 */
#ifndef _http11_common_h
#define _http11_common_h

#include <sys/types.h>

/**
 * @brief 定义一个元素回调函数类型
 * 
 * 该回调函数用于处理元素数据，当遇到特定元素时会被调用。
 * 
 * @param data 回调函数的上下文数据指针
 * @param at 元素数据的起始地址
 * @param length 元素数据的长度
 */
typedef void (*element_cb)(void *data, const char *at, size_t length);

/**
 * @brief 定义一个字段回调函数类型
 * 
 * 该回调函数用于处理字段数据，当遇到特定字段时会被调用。
 * 
 * @param data 回调函数的上下文数据指针
 * @param field 字段名的起始地址
 * @param flen 字段名的长度
 * @param value 字段值的起始地址
 * @param vlen 字段值的长度
 */
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);

#endif
