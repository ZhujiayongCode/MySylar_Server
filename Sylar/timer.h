/**
 * @file iomanager.h
 * @brief 定时器封装
 * @author Zhujiayong
 * @email zhujiayong_hhh@163.com
 * @copyright Copyright (c) 2025年 Zhujiayong All rights reserved 
 **/
#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <list>
#include "thread.h"

namespace Sylar {

class TimerManager;

/**
 * @brief 获取单调时钟时间（毫秒）
 * @return 返回从系统启动开始计算的毫秒数
 */
uint64_t GetMonotonicMS();

/**
 * @brief 获取系统时钟时间（毫秒）
 * @return 返回系统时钟的毫秒数
 */
uint64_t GetSystemMS();

/**
 * @brief 时间轮槽
 */
struct TimerSlot {
    /// 定时器列表
    std::list<std::shared_ptr<Timer>> timers;
    /// 槽的过期时间
    uint64_t expiration;
};

/**
 * @brief 时间轮
 */
class TimeWheel {
public:
    /**
     * @brief 构造函数
     * @param[in] slot_size 时间轮槽数量
     * @param[in] tick_ms 每个槽的时间间隔(毫秒)
     */
    TimeWheel(size_t slot_size, uint64_t tick_ms);

    /**
     * @brief 添加定时器
     * @param[in] timer 定时器
     * @return 是否添加成功
     */
    bool addTimer(std::shared_ptr<Timer> timer);

    /**
     * @brief 获取下一个定时器执行时间
     * @return 返回距离下一个定时器执行的时间(毫秒)
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取到期的定时器
     * @param[out] expired 到期的定时器列表
     */
    void getExpiredTimers(std::vector<std::shared_ptr<Timer>>& expired);

    /**
     * @brief 清空时间轮
     */
    void clear();

private:
    /// 时间轮槽数量
    size_t m_slotSize;
    /// 每个槽的时间间隔(毫秒)
    uint64_t m_tickMs;
    /// 当前槽位置
    size_t m_currentSlot;
    /// 时间轮槽数组
    std::vector<TimerSlot> m_slots;
    /// 读写锁
    RWMutex m_mutex;
};

/**
 * @brief 定时器
 */
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
friend class TimeWheel;
public:
    /// 定时器的智能指针类型
    typedef std::shared_ptr<Timer> ptr;

    /**
     * @brief 定时器比较器
     */
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

    /**
     * @brief 取消定时器
     */
    bool cancel();

    /**
     * @brief 刷新设置定时器的执行时间
     */
    bool refresh();

    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms, bool from_now);
private:
    /**
     * @brief 构造函数
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 回调函数
     * @param[in] recurring 是否循环
     * @param[in] manager 定时器管理器
     */
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    /**
     * @brief 构造函数
     * @param[in] next 执行的时间戳(毫秒)
     */
    Timer(uint64_t next);
private:
    /// 是否循环定时器
    bool m_recurring = false;
    /// 执行周期，即执行的时间间隔(毫秒)，相对时间
    uint64_t m_ms = 0;
    /// 精确的执行时间，使用单调时钟
    uint64_t m_next = 0;
    /// 回调函数
    std::function<void()> m_cb;
    /// 定时器管理器
    TimerManager* m_manager = nullptr;
};

/**
 * @brief 定时器管理器
 */
class TimerManager {
friend class Timer;
public:
    /// 读写锁类型
    typedef RWMutex RWMutexType;

    /**
     * @brief 构造函数
     */
    TimerManager();

    /**
     * @brief 析构函数
     */
    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] recurring 是否循环定时器
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        ,bool recurring = false);

    /**
     * @brief 添加条件定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] weak_cond 条件
     * @param[in] recurring 是否循环
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                        ,std::weak_ptr<void> weak_cond
                        ,bool recurring = false);

    /**
     * @brief 到最近一个定时器执行的时间间隔(毫秒)
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取需要执行的定时器的回调函数列表
     * @param[out] cbs 回调函数数组
     */
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    /**
     * @brief 是否有定时器
     */
    bool hasTimer();
protected:

    /**
     * @brief 当有新的定时器插入到定时器的首部,执行该函数
     */
    virtual void onTimerInsertedAtFront() = 0;

    /**
     * @brief 将定时器添加到管理器中
     */
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
    /**
     * @brief 检测时间异常
     * @return 是否检测到时间异常
     */
    bool detectTimeAnomaly();
private:
    /// Mutex
    RWMutexType m_mutex;
    /// 时间轮
    TimeWheel m_timeWheel;
    /// 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    /// 上次单调时钟时间
    uint64_t m_lastMonotonicTime = 0;
    /// 上次系统时间
    uint64_t m_lastSystemTime = 0;
};

}

#endif
