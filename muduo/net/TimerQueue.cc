#define __STDC_LIMIT_MACROS

#include <boost/bind.hpp>
#include <sys/timerfd.h>

#include <muduo/base/Logging.h>
#include <muduo/net/TimerQueue.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Timer.h>
#include <muduo/net/TimerId.h>

namespace muduo
{
    namespace net
    {
        namespace detail
        {
            // 创建定时器
            int createTimerfd()
            {
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
                if (timerfd < 0)
                {
                    LOG_SYSFATAL << "Failed in timerfd_create";
                }
                return timerfd;
            }

            // 计算超时时刻与当前时间的时间差
            struct timespec howMuchTimeFromNow(Timestamp when)
            {
                int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
                if (microseconds < 100) // 精度不需要太高, 100微妙足够了.
                {
                    microseconds = 100;
                }

                struct timespec ts;
                ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
                return ts;
            }

            // 清除定时器(把数据读出来), 避免一直触发
            void readTimerfd(int timerfd, Timestamp now)
            {
                uint64_t howmany;
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if (n != sizeof howmany)
                {
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }

            // 重置定时器的超时时刻
            // 参数expiration: 超时时刻
            void resetTimerfd(int timerfd, Timestamp expiration)
            {
                // wake up loop by timerfd_settime()
                struct itimerspec newValue;
                struct itimerspec oldValue;
                bzero(&newValue, sizeof newValue);
                bzero(&oldValue, sizeof oldValue);
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
                if (ret)
                {
                    LOG_SYSERR << "timerfd_settime()";
                }
            }

        } // namespace detail
    }     // namespace net
} // namespace muduo

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(boost::bind(&TimerQueue::handleRead, this));

    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (TimerList::iterator it = timers_.begin();
         it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

// 参数when: 多久后调用cb
// 参数interval: 没隔多久调用一次cb
TimerId TimerQueue::addTimer(const TimerCallback &cb,
                             Timestamp when,
                             double interval)
{
    Timer *timer = new Timer(cb, when, interval);
    loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
}

// 被addTimer()所调用, 在addTimer()中, 该函数作为loop的runInLoop()参数, 是在IO线程中调用的, 所以对timers_的修改不需要加锁.
void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread(); // 保证不能跨线程调用

    // 插入一个定时器, 有可能会使得最早到期的定时器发生改变
    bool earliestChanged = insert(timer); // 如果timer比TimerQueue中管理的所有定时器都要早, 会使得最早到期的定时器发生改变, 则返回true.

    if (earliestChanged)
    {
        // 重置定时器的超时时刻(timerfd_settime)
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 取消一个定时器timerId:
// 1. 待取消的定时器在timers_和activeTimers_中, 直接取消.
// 2. "自注销"的情况, 待取消的定时器在handleRead中已经从timers_和activeTimers_取出来了, 而此刻取消定时器的操作又是handleRead()触发的.
//    此时只要把定时器加入到cancelingTimers_中就可以了, 等待handleRead()之后调用reset()来取消.
void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    auto it = activeTimers_.find(timer); // 查找该定时器
    if (it != activeTimers_.end())       // 找到之后就删除
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;

        activeTimers_.erase(it);
        delete it->first; // 如果用了unique_ptr, 这里就不需要手动删除了
    }
    else if (callingExpiredTimers_) // "自注销"的情况.
    {
        // 加入到cancelingTimers_, 见handleRead(), 等定时器回调函数调用完毕, 在reset()中清理(一次性定时器, 或者已被取消的定时器, 删除该定时器.).
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

// 定时器可读时(超时)的回调函数
// 取出当前的定时器, 并回调这些定时器的回调函数.
void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();

    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now); // 清除该事件, 避免一直触发

    // 获取该超时时刻之前所有的定时器列表
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        // 这里回调定时器处理函数
        it->second->run();
    }
    // 不是一次性定时器，需要重启
    callingExpiredTimers_ = false;

    reset(expired, now);
}

// 从activeTimers_中返回参数now之前(超时)所有的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());

    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX)); // 哨兵: 保证了在set中查找时, 即使first相等, second也小于该sentry.

    // 返回第一个未到期的Timer的迭代器. 注意: now < end->first, 而不是 >=, 因为sentry.second是UINTPTR_MAX
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    // 将所有到期的定时器插入到expired中
    std::copy(timers_.begin(), end, std::back_inserter(expired));

    // 从timers_中移除所有到期的定时器
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中移除所有到期的定时器
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }

    assert(timers_.size() == activeTimers_.size());

    return expired; // RVO优化: 就是少了一个临时变量的拷贝构造和析构, VS2019中Debug没有RVO优化, Release有.
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        // 如果是重复的定时器, 并且是未取消定时器, 则重启该定时器
        if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            // 一次性定时器, 或者已被取消的定时器, 删除该定时器.
            // FIXME move to a free list
            delete it->second; // FIXME: no delete please
        }
    }

    if (!timers_.empty())
    {
        // 获取最早到期的定时器超时时间
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        // 重置定时器的超时时刻(timerfd_settime)
        resetTimerfd(timerfd_, nextExpire);
    }
}

// 将定时器timer插入到timers_/ActiveTimer中
bool TimerQueue::insert(Timer *timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    // 最早到期时间是否改变
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    // 如果timers_为空, 或者when小于timers_中的最早到期时间
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    // 插入到timers_中
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }

    // 插入到activeTimers_中
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());

    return earliestChanged;
}
