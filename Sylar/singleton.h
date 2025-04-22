/**
 * @file singleton.h
 * @brief 单例模式封装（线程安全，基于C++11的std::atomic实现）
 */

#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>
#include <atomic>
#include <mutex>

namespace Sylar {

/**
 * @brief 单例模式封装类（线程安全）
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     */
    static T* GetInstance() {
        T* instance = m_instance.load(std::memory_order_acquire);//加载示例，保证后续
        if (!instance) {
            std::lock_guard<std::mutex> lock(m_mutex);
            instance = m_instance.load(std::memory_order_relaxed);
            if (!instance) {
                instance = new T();
                m_instance.store(instance, std::memory_order_release);
            }
        }
        return instance;
    }

    /**
     * @brief 销毁单例实例
     */
    static void DestroyInstance() {
        T* instance = m_instance.load(std::memory_order_acquire);
        if (instance) {
            std::lock_guard<std::mutex> lock(m_mutex);
            instance = m_instance.load(std::memory_order_relaxed);
            if (instance) {
                delete instance;
                m_instance.store(nullptr, std::memory_order_release);
            }
        }
    }

private:
    static std::atomic<T*> m_instance; ///< 单例实例
    static std::mutex m_mutex;         ///< 线程安全互斥锁
};

// 静态成员变量定义
template<class T, class X, int N>
std::atomic<T*> Singleton<T, X, N>::m_instance{nullptr};

template<class T, class X, int N>
std::mutex Singleton<T, X, N>::m_mutex;

/**
 * @brief 单例模式智能指针封装类（线程安全）
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance() {
        std::shared_ptr<T> instance = m_instance.load(std::memory_order_acquire);
        if (!instance) {
            std::lock_guard<std::mutex> lock(m_mutex);
            instance = m_instance.load(std::memory_order_relaxed);
            if (!instance) {
                instance = std::make_shared<T>();
                m_instance.store(instance, std::memory_order_release);
            }
        }
        return instance;
    }

    /**
     * @brief 销毁单例实例
     */
    static void DestroyInstance() {
        std::shared_ptr<T> instance = m_instance.load(std::memory_order_acquire);
        if (instance) {
            std::lock_guard<std::mutex> lock(m_mutex);
            instance = m_instance.load(std::memory_order_relaxed);
            if (instance) {
                m_instance.store(nullptr, std::memory_order_release);
            }
        }
    }

private:
    static std::atomic<std::shared_ptr<T>> m_instance; ///< 单例智能指针实例
    static std::mutex m_mutex;                        ///< 线程安全互斥锁
};

// 静态成员变量定义
template<class T, class X, int N>
std::atomic<std::shared_ptr<T>> SingletonPtr<T, X, N>::m_instance{nullptr};

template<class T, class X, int N>
std::mutex SingletonPtr<T, X, N>::m_mutex;

} // namespace sylar

#endif