/**
 * @file zlib_stream.h
 * @brief 基于 zlib 库实现的流处理类，用于数据的压缩和解压缩操作。
 * @author Zhujiyaong
 * @date 2025-04-11
 */

#ifndef __SYLAR_STREAMS_ZLIB_STREAM_H__
#define __SYLAR_STREAMS_ZLIB_STREAM_H__

#include "Sylar/stream.h"
#include <sys/uio.h>
#include <zlib.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>

namespace Sylar {

/**
 * @class ZlibStream
 * @brief 基于 zlib 库实现的流类，用于对数据进行压缩和解压缩操作。
 * @details 该类继承自 Stream 类，提供了多种压缩算法和压缩级别供选择，
 *          支持 GZIP、ZLIB 和 DEFLATE 等格式的压缩和解压缩。
 */
class ZlibStream : public Stream {
public:
    typedef std::shared_ptr<ZlibStream> ptr;

    enum Type {
        ZLIB,   ///< ZLIB 压缩算法
        DEFLATE, ///< DEFLATE 压缩算法
        GZIP    ///< GZIP 压缩算法
    };

    enum Strategy {
        DEFAULT = Z_DEFAULT_STRATEGY, ///< 默认压缩策略
        FILTERED = Z_FILTERED, ///< 过滤数据的压缩策略
        HUFFMAN = Z_HUFFMAN_ONLY, ///< 仅使用哈夫曼编码的压缩策略
        FIXED = Z_FIXED, ///< 固定哈夫曼编码的压缩策略
        RLE = Z_RLE ///< 行程编码的压缩策略
    };

    enum CompressLevel {
        NO_COMPRESSION = Z_NO_COMPRESSION, ///< 不进行压缩
        BEST_SPEED = Z_BEST_SPEED, ///< 最快压缩速度
        BEST_COMPRESSION = Z_BEST_COMPRESSION, ///< 最高压缩率
        DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION ///< 默认压缩级别
    };

    /**
     * @brief 创建一个用于 GZIP 压缩或解压缩的 ZlibStream 实例。
     * @param encode 是否进行压缩操作，true 表示压缩，false 表示解压缩。
     * @param buff_size 缓冲区大小，默认为 4096 字节。
     * @return 返回一个指向 ZlibStream 实例的智能指针。
     */
    static ZlibStream::ptr CreateGzip(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateZlib(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateDeflate(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr Create(bool encode, uint32_t buff_size = 4096,
            Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15
            ,int memlevel = 8, Strategy strategy = DEFAULT);

    ZlibStream(bool encode, uint32_t buff_size = 4096);
    ~ZlibStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    int flush();

    bool isFree() const { return m_free;}
    void setFree(bool v) { m_free = v;}

    bool isEncode() const { return m_encode;}
    void setEndcode(bool v) { m_encode = v;}

    std::vector<iovec>& getBuffers() { return m_buffs;}
    std::string getResult() const;
    Sylar::ByteArray::ptr getByteArray();
private:
    int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION
             ,int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

    int encode(const iovec* v, const uint64_t& size, bool finish);
    int decode(const iovec* v, const uint64_t& size, bool finish);
private:
    z_stream m_zstream; ///< zlib 流结构体，用于管理压缩和解压缩操作。
    uint32_t m_buffSize; ///< 缓冲区大小。
    bool m_encode; ///< 是否进行压缩操作的标志，true 表示压缩，false 表示解压缩。
    bool m_free; ///< 流是否已释放资源的标志。
    std::vector<iovec> m_buffs; ///< 存储数据的缓冲区向量。
};

}

#endif
