#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include <boost/static_assert.hpp>

#include <muduo/base/Logging.h>
#include <muduo/net/poller/EPollPoller.h>
#include <muduo/net/Channel.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4) are expected to be the same.

BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

// Channel::index的取值
namespace
{
    const int kNew = -1;    // Channel 不在epoll树上, 也不在channels_中
    const int kAdded = 1;   // Channel 在epoll树上, 也在channels_中
    const int kDeleted = 2; // Channel 不在epoll树上, 在channels_中.
} // namespace

// 创建epoll树
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)), // fork()创建的子进程在子进程中关闭该socket, 即子进程不会继承.
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

// 关闭epfd
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 把当前(最多等待timeoutMs毫秒)就绪fd对应的channel(用event.data.ptr保存)放到参数activeChannels中去, 并返回当前的时间.
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
                                 events_.data(), //  &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);

    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";

        fillActiveChannels(numEvents, activeChannels);
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2); // 初始events_大小为16, 当不够时再加倍增加.
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << " nothing happended";
    }
    else
    {
        LOG_SYSERR << "EPollPoller::poll()";
    }
    return now;
}

// 就绪fd对应的channel(用event.data.ptr保存)放到参数activeChannels中去.
// 调用: EPollPoller::poll()
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());

    // 不能一边遍历evnets_, 一边调用Channel::handleEvent(), 因为: 
    // 1. 因为后者会添加或删除Channel, 从而操作events_在遍历期间改变大小.
    // 2. Poller的职责是只负责IO multiplexing, 不负责事件分发.
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 该函数的最上层的调用方是Channel的enableXXX()或disableXXX(), 参数channel就是调用该函数的Channel.
// 根据Channel的index, 设置Channel epoll树和channels_的操作:
//   1. 不在channels_中就添加进去.
//   2. 不在epoll树上, 就上树.
//   3. 在epoll树上, 就MOD/DEL.
void EPollPoller::updateChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();

    const int index = channel->index();
    if (index == kNew || index == kDeleted) // 不在epoll树上.
    {
        int fd = channel->fd();
        if (index == kNew) // 不在channels_
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // 在channels_
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);

        // 指定到此处, 该channel的index_为kAdd.
    }
    else // index == kAdded
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        if (channel->isNoneEvent()) // 没有事件, 就从epoll树中删除
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 把channel从epoll树上和channels中移除, 移除之后channel的index就是kNew.
void EPollPoller::removeChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    assert(channel->isNoneEvent());

    int fd = channel->fd(); // 保证在channels中
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);

    int index = channel->index();
    assert(index == kAdded || index == kDeleted); // 保证在channels中

    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 就是包装了epoll_ctl(), 被updateChannel()和removeChannel()所调用.
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;
        }
        else
        {
            LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
        }
    }
}
