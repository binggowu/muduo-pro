#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

struct epoll_event;

namespace muduo
{
    namespace net
    {
        // Poller是EventLoop的间接成员, 只供owner EventLoop在IO线程中调用, 因此无须加锁.
        // Poller的生命周期和EventLoop相等.
        class EPollPoller : public Poller
        {
            typedef std::vector<struct epoll_event> EventList; // muduo使用了epoll_event.data的ptr成员.
            typedef std::map<int, Channel *> ChannelMap;       // fd到Channel*的映射

        public:
            EPollPoller(EventLoop *loop);
            virtual ~EPollPoller();

            virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels);
            virtual void updateChannel(Channel *channel);
            virtual void removeChannel(Channel *channel);

        private:
            static const int kInitEventListSize = 16;

            void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

            void update(int operation, Channel *channel);

            int epollfd_;
            EventList events_; // 用于保存就绪的事件, 即epoll_wait()的第二个参数.

            // 1. Poller自己并不拥有Channel, Channel在析构之前(channels_), 必须要unregister(EventLoop::removeChannel), 避免空悬指针.
            // 2. 根据channel的index的含义, 位于channels中的Channel一定在epoll树上, 反之不一定.
            ChannelMap channels_;
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_POLLER_EPOLLPOLLER_H
