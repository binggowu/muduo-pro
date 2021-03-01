#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;

        // 该类会启动一个子线程 thread_, 并在其中运行 loop_的loop(). 说明IO线程不是主线程.
        class EventLoopThread : boost::noncopyable
        {
        public:
            typedef boost::function<void(EventLoop *)> ThreadInitCallback;

            EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
            ~EventLoopThread();
            
            EventLoop *startLoop(); 

        private:
            void threadFunc();

            EventLoop *loop_; // 栈上的对象. 平时就在loop()中, 一旦quit为true, 就退出loop(), 栈对象自动销毁.
            bool exiting_;
            Thread thread_;
            MutexLock mutex_;
            Condition cond_;
            ThreadInitCallback callback_; // 回调函数在 EventLoop::loop()之前被调用
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_EVENTLOOPTHREAD_H
