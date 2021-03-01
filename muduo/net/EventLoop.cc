#include <signal.h>
#include <sys/eventfd.h>
#include <boost/bind.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Singleton.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/SocketsOps.h>
#include <muduo/net/TimerQueue.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
    // 一个线程只有一个EventLoop对象, 属于线程私有的, 所以用__thread修饰.
    __thread EventLoop *t_loopInThisThread = 0;

    // 单位毫秒, epoll_wait()等待的使用.
    const int kPollTimeMs = 10000;

    // 创建EventPool::wakeupFd_, 仅在EventPool构造函数中被调用
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_SYSERR << "Failed in eventfd";
            abort();
        }
        return evtfd;
    }

    // 如果服务器繁忙, 没有及时处理对方断开连接的事件, 就可能出现在客户端连接断开后继续发送数据的情况.

#pragma GCC diagnostic ignored "-Wold-style-cast"
    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            LOG_TRACE << "Ignore SIGPIPE";
        }
    };
#pragma GCC diagnostic error "-Wold-style-cast"

    IgnoreSigPipe initObj;
} // namespace

// 返回当前线程的EventLoop对象指针, 如果当前线程没有EventLoop, 就返回nullptr.
EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false), 
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL)
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread) // 如果已经创建, 终止(LOG_FATAL). one loop pre thread: 一个线程只有一个EventLoop, 不可以重复创建多个.
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(boost::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

// 事件循环, 不能跨线程调用, 只能在创建该对象的IO线程中调用.
// (1) 调用Poller::poll()获得当前活跃Channel列表
// (2) 依次调用每个Channel的handEvent()函数.
// (3) 执行当IO线程的一些计算任务.
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    LOG_TRACE << "EventLoop " << this << " start looping";

    looping_ = true;
    quit_ = false;
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); // IO线程平时就阻塞在这里
        ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }

        // TODO sort channel by priority
        eventHandling_ = true;
        for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_); // 处理事件回调函数
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;

        doPendingFunctors(); // 当IO线程也能执行一些计算任务.
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 线程安全
// quit()不是中断或signal, 而是设置标志, 所以不是立刻起效, 而是在EventLoop::loop()下一次检查while(!quit_)时起效.
void EventLoop::quit()
{
    quit_ = true; // loop()函数就不会再进入while循环.

    if (!isInLoopThread()) // 如果不是由IO线程调用的, 那么IO线程大概率在(poller_->poll), 此时需要唤醒.
    {
        wakeup();
    }
}

// 该函数可以轻松的实现: 不能跨线程调用的函数做到 线程安全 的调用.
void EventLoop::runInLoop(const Functor &cb)
{
    if (isInLoopThread()) // 如果是当前IO线程调用runInLoop, 则同步调用cb
    {
        cb();
    }
    else // 如果是其它线程调用runInLoop, 则异步地将cb添加到队列
    {
        queueInLoop(cb);
    }
}

// 将回调函数cb添加到队列pendingFunctors_, 实现异步调用cb.
void EventLoop::queueInLoop(const Functor &cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    // 需要唤醒:
    //   1) 调用 queueInLoop() 的线程不是IO线程
    //   2) doPendingFunctors()中的函数调用了queueInLoop()函数, 唤醒之后, 这样新增的cb就能及时调用.
    // 不需要唤醒: 只有当前IO线程的事件回调中调用queueInLoop.
    if (!isInLoopThread() || callingPendingFunctors_) // callingPendingFunctors_ 在doPendingFunctors()中设置为true.
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb)
{
    return timerQueue_->addTimer(cb, time, 0.0); // 一次性定时器
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), delay)); // 一次性定时器
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), interval)); // 持续性定时器
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

// 从Poller中添加/更新通道
// Channel中保存了EventLoop对象loop_, 该函数在Channel中通过loop_调用的.
void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    poller_->updateChannel(channel);
}

// 从Poller中移除通道
void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid();
}

// 通过 eventfd 来唤醒IO线程. 例如线程是调用quit()时, IO线程大概率会阻塞在poller_->poll(), 该函数正好可以唤醒IO线程从poller_->poll()中返回.
void EventLoop::wakeup()
{
    uint64_t one = 1; // eventfd的buffer大小是定长8字节
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

// wakeupChannel_的回调函数, 往eventfd中读数据.
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

// 调用 pendingFunctors_ 中的函数
void EventLoop::doPendingFunctors()
{
    // 不是简单地在临界区内依次调用Functor, 而是swap到functors中调用:
    //   1) 减小临界区的大小.
    //   2) 避免死锁, 因为functor可能再次调用queueInLoop(), 对mutex_再次加锁.
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    // 有可能上面刚刚 swap pendingFunctors_ 之后, pendingFunctors_ 中又添加了cb.
    // 即使这样也不要反复执行 doPendingFunctors() 直到pendingFunctors 为空, 否则IO线程可能陷入死循环, 无法处理IO事件.

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
        // functors[i]()可能调用queueInLoop(), 这时queueInLoop()就必须wakeup(), 否则新增的cb可能就不能及时调用了 .
    }
    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
    for (ChannelList::const_iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
    {
        const Channel *ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "} ";
    }
}
