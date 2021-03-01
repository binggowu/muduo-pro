#include <stdlib.h>

#include <muduo/net/Poller.h>
#include <muduo/net/poller/PollPoller.h>
#include <muduo/net/poller/EPollPoller.h>

using namespace muduo::net;

// 通过环境变量 MUDUO_USE_POLL 来选择poll还是epoll(默认).
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return new PollPoller(loop);
    }
    else
    {
        return new EPollPoller(loop);
    }
}
