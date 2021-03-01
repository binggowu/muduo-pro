#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread;

        class EventLoopThreadPool : boost::noncopyable
        {
        public:
            typedef boost::function<void(EventLoop *)> ThreadInitCallback;

            EventLoopThreadPool(EventLoop *baseLoop);
            ~EventLoopThreadPool();

            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            void start(const ThreadInitCallback &cb = ThreadInitCallback());
            EventLoop *getNextLoop();

        private:
            EventLoop *baseLoop_; // 与Acceptor所属EventLoop相同, 见TcpServer::TcpServer()
            bool started_;        // 是否已经启动, 见 start()
            int numThreads_;      // 线程数
            int next_;            // 新连接到来, 所选择的 EventLoop对象下标

            boost::ptr_vector<EventLoopThread> threads_; // IO线程列表, IO线程都是子线程.
            std::vector<EventLoop *> loops_;             // IO线程列表中的EventLoop对象, 都是栈上的对象, 见 EventLoopThread::threadFunc()
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_EVENTLOOPTHREADPOOL_H
