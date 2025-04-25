#include "timer.h"
#include "util.h"
#include <time.h>

namespace Sylar {

uint64_t GetMonotonicMS() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t GetSystemMS() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

TimeWheel::TimeWheel(size_t slot_size, uint64_t tick_ms)
    : m_slotSize(slot_size)
    , m_tickMs(tick_ms)
    , m_currentSlot(0) {
    m_slots.resize(slot_size);
    for(auto& slot : m_slots) {
        slot.expiration = 0;
    }
}

bool TimeWheel::addTimer(std::shared_ptr<Timer> timer) {
    RWMutex::WriteLock lock(m_mutex);
    uint64_t now = GetMonotonicMS();
    if(timer->m_next < now) {
        return false;
    }

    // 计算定时器应该放在哪个槽
    uint64_t diff = timer->m_next - now;
    size_t ticks = diff / m_tickMs;
    size_t slot = (m_currentSlot + ticks) % m_slotSize;
    
    // 设置槽的过期时间
    m_slots[slot].expiration = timer->m_next;
    m_slots[slot].timers.push_back(timer);
    return true;
}

uint64_t TimeWheel::getNextTimer() {
    RWMutex::ReadLock lock(m_mutex);
    uint64_t now = GetMonotonicMS();
    uint64_t next = ~0ull;

    for(size_t i = 0; i < m_slotSize; ++i) {
        size_t slot = (m_currentSlot + i) % m_slotSize;
        if(!m_slots[slot].timers.empty()) {
            if(m_slots[slot].expiration <= now) {
                return 0;
            }
            next = std::min(next, m_slots[slot].expiration - now);
        }
    }

    return next;
}

void TimeWheel::getExpiredTimers(std::vector<std::shared_ptr<Timer>>& expired) {
    RWMutex::WriteLock lock(m_mutex);
    uint64_t now = GetMonotonicMS();

    while(true) {
        auto& slot = m_slots[m_currentSlot];
        if(slot.expiration > now) {
            break;
        }

        expired.insert(expired.end(), slot.timers.begin(), slot.timers.end());
        slot.timers.clear();
        slot.expiration = 0;
        m_currentSlot = (m_currentSlot + 1) % m_slotSize;
    }
}

void TimeWheel::clear() {
    RWMutex::WriteLock lock(m_mutex);
    for(auto& slot : m_slots) {
        slot.timers.clear();
        slot.expiration = 0;
    }
    m_currentSlot = 0;
}

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if(!lhs && !rhs) {
        return false;
    }
    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }
    if(lhs->m_next < rhs->m_next) {
        return true;
    }
    if(rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
    m_next = GetMonotonicMS() + m_ms;
}

Timer::Timer(uint64_t next)
    :m_next(next) {
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    m_next = GetMonotonicMS() + m_ms;
    return m_manager->m_timeWheel.addTimer(shared_from_this());
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    uint64_t start = 0;
    if(from_now) {
        start = GetMonotonicMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    return m_manager->m_timeWheel.addTimer(shared_from_this());
}

TimerManager::TimerManager()
    : m_timeWheel(60, 1000) {  // 60个槽，每个槽1秒
    m_lastMonotonicTime = GetMonotonicMS();
    m_lastSystemTime = GetSystemMS();
}

TimerManager::~TimerManager() {
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                                  ,bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                                    ,std::weak_ptr<void> weak_cond
                                    ,bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    return m_timeWheel.getNextTimer();
}

bool TimerManager::detectTimeAnomaly() {
    uint64_t currentMonotonic = GetMonotonicMS();
    uint64_t currentSystem = GetSystemMS();
    
    // 计算时间差
    uint64_t monotonicDiff = currentMonotonic - m_lastMonotonicTime;
    uint64_t systemDiff = currentSystem - m_lastSystemTime;
    
    // 更新上次时间
    m_lastMonotonicTime = currentMonotonic;
    m_lastSystemTime = currentSystem;
    
    // 如果时间差差异超过1秒，认为发生时间异常
    return std::abs((int64_t)(monotonicDiff - systemDiff)) > 1000;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_timeWheel.getExpiredTimers(expired);
    }

    cbs.reserve(expired.size());
    uint64_t now_ms = GetMonotonicMS();

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            RWMutexType::WriteLock lock(m_mutex);
            m_timeWheel.addTimer(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    bool at_front = m_timeWheel.addTimer(val);
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return m_timeWheel.getNextTimer() != ~0ull;
}

}
