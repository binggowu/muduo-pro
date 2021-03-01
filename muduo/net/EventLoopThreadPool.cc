#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop)
    : baseLoop_(baseLoop),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // 不需要销毁 loops_
}

// 启动线程池
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        EventLoopThread *t = new EventLoopThread(cb);
        threads_.push_back(t);
        loops_.push_back(t->startLoop()); // 启动 EventLoopThread线程, 在进入事件循环之前, 会调用cb
    }

    if (numThreads_ == 0 && cb) // 只有一个EventLoop, 在这个 EventLoop进入事件循环之前, 调用cb
    {
        cb(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();

    EventLoop *loop = baseLoop_;

    // 如果loops_为空, 则loop指向baseLoop_
    // 如果不为空, 从线程池中选择一个EventLoop
    if (!loops_.empty())
    {
        loop = loops_[next_];

        // 更新 next_变量
        ++next_;
        if (implicit_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}
