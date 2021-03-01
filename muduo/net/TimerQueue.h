#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Channel.h>

#include <set>
#include <vector>
#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class Timer;
        class TimerId;

        // 内部类, 供EventLoop使用.
        class TimerQueue : boost::noncopyable
        {
        public:
            TimerQueue(EventLoop *loop);
            ~TimerQueue();

            // 不直接调用, 而是通过EventLoop的runAt/runAfter/runEvery, cancel所调用.
            TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);
            void cancel(TimerId timerId);

        private:
            typedef std::pair<Timestamp, Timer *> Entry; // 允许有两个定时器的Timestamp是相同的
            typedef std::set<Entry> TimerList;

            typedef std::pair<Timer *, int64_t> ActiveTimer;
            typedef std::set<ActiveTimer> ActiveTimerSet;

            // 以下2个成员函数只可能在其所属的I/O线程中调用, 因而不必加锁.
            void addTimerInLoop(Timer *timer);
            void cancelInLoop(TimerId timerId);

            void handleRead();

            std::vector<Entry> getExpired(Timestamp now);

            // 重置超时的定时器列表
            void reset(const std::vector<Entry> &expired, Timestamp now);

            // 插入定时器
            bool insert(Timer *timer);

            EventLoop *loop_; // 所属EventLoop
            const int timerfd_;
            Channel timerfdChannel_; // 关注timerfd上的可读事件

            // timers_与activeTimers_保存的是相同的数据(目前有效的Timer_*), size相同, 但排序不同. 注意: 在delete Timer*资源时, 只要对其中一个操作即可, 不要两个都操作.
            TimerList timers_;            // 是按到期时间排序
            ActiveTimerSet activeTimers_; // 是按对象地址排序

            // 如下2个成员变量是为了应对"自注销"的情况, 即定时器回调中注销定时器自己, 详见cancelInLoop().

            bool callingExpiredTimers_;      // 是否正在处理超时定时器
            ActiveTimerSet cancelingTimers_; // 保存的是被取消的定时器
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TIMERQUEUE_H
