#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TimerId.h>

namespace muduo
{
    namespace net
    {
        class Channel;
        class Poller;
        class TimerQueue;

        // 事件循环类, 从poll()返回到再次阻塞到poll()称为"一次事件循环".
        // 一个线程最有只能有一个该类(one loop pre thread). 创建该EventLoop的线程, 称为IO线程. IO线程不一定是主线程, 任何一个线程都可以创建并运行EventLoop.
        class EventLoop : boost::noncopyable
        {
        public:
            typedef std::vector<Channel *> ChannelList;
            typedef boost::function<void()> Functor;

            EventLoop();
            ~EventLoop(); // force out-line dtor, for scoped_ptr members.

            void loop();

            void quit();

            ///
            /// Time when poll returns, usually means data arrivial.
            ///
            Timestamp pollReturnTime() const { return pollReturnTime_; }

            int64_t iteration() const { return iteration_; }

            // 函数调用

            void runInLoop(const Functor &cb);
            void queueInLoop(const Functor &cb);

            // 定时器

            // 在某个时刻运行定时器, 线程安全.
            TimerId runAt(const Timestamp &time, const TimerCallback &cb);

            // 过一段时间运行定时器, 线程安全.
            TimerId runAfter(double delay, const TimerCallback &cb);

            // 每隔一段时间运行定时器, 线程安全.
            TimerId runEvery(double interval, const TimerCallback &cb);

            // 取消定时器, 线程安全.
            void cancel(TimerId timerId);

            // internal usage
            void wakeup();

            // Poller

            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);

            // pid_t threadId() const { return threadId_; }

            // 断言当前线程是不是创建该对象的线程
            void assertInLoopThread()
            {
                if (!isInLoopThread())
                {
                    abortNotInLoopThread();
                }
            }

            // 判断当前线程是不是创建该对象的线程
            bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

            // bool callingPendingFunctors() const { return callingPendingFunctors_; }
            bool eventHandling() const { return eventHandling_; }

            static EventLoop *getEventLoopOfCurrentThread();

        private:
            void abortNotInLoopThread();
            void doPendingFunctors();

            void printActiveChannels() const; // DEBUG

            boost::scoped_ptr<Poller> poller_; // 不能放在timerQueue_下面, 因为创建timerQueue_时会用到Channel, Channel会调用poller, 所以poller_要先创建.
            ChannelList activeChannels_;       // 当前所有就绪event中的channel, poller返回的活动channel
            Channel *currentActiveChannel_;    // 指向activeChannels_中某一个, 遍历用

            // 状态变量

            bool looping_;       // 是否处理事件循环中, 只在loop()被使用.
            bool quit_;          // 是否退出标志
            bool eventHandling_; // 是否事件处理的状态

            int64_t iteration_;        // 记录poll()调用了多少次
            const pid_t threadId_;     // 当前对象所属线程ID
            Timestamp pollReturnTime_; // 调用poll()所返回的时间戳, 只是在loop函数中使用, 感觉没必要做数据成员.

            boost::scoped_ptr<TimerQueue> timerQueue_; // 定时器

            // IO线程自己的任务

            bool callingPendingFunctors_;          // 状态变量, 是否正在执行doPendingFunctors(), 仅仅在该函数中设置.
            std::vector<Functor> pendingFunctors_; // 回调函数队列, 会被暴露给其他线程, 所以需要mutex_
            MutexLock mutex_;                      // 用于对pendingFunctors_加锁, 在 doPendingFunctors()和 queueInLoop()中被使用, 注意避免死锁.

            // Wake Up 机制

            int wakeupFd_;                             // eventfd, 由 createEventfd()创建
            boost::scoped_ptr<Channel> wakeupChannel_; // 用于处理 eventFd_上的readable事件, 将事件分发给handleRead()函数.
            void handleRead();                         //  wakeupChannel_ 的读回调函数
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_EVENTLOOP_H
