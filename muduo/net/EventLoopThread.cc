#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb)
    : loop_(NULL),
      exiting_(false),
      thread_(boost::bind(&EventLoopThread::threadFunc, this)),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    loop_->quit(); // 让IO线程的loop循环退出, 从而退出了IO线程
    thread_.join();
}

// 创建并启动子线程 thread_ -> 等待 loop_在 thread_中被赋值 -> 最后返回该loop_
EventLoop *EventLoopThread::startLoop()
{
    assert(!thread_.started());

    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL) // 等待线程函数(threadFunc)创建EventLoop
        {
            cond_.wait();
        }
    }

    return loop_;
}

// 先调用callbak_ -> 赋值loop_(EventLoop), 并通知loop已经创建 -> 进入loop().
void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();

        // loop_指针指向了一个栈上的对象, threadFunc函数退出之后, 这个指针就失效了.
        // threadFunc()退出, 就意味着线程退出了, EventLoopThread对象也就没有存在的价值了, 因而不会有什么大的问题
    }

    loop.loop();
    //assert(exiting_);
}
