#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

// 并没有include <poll.h>, 而是前向声明.
struct pollfd;

namespace muduo
{
    namespace net
    {
        ///
        /// IO Multiplexing with poll(2).
        ///
        class PollPoller : public Poller
        {
            typedef std::vector<struct pollfd> PollFdList;
            typedef std::map<int, Channel *> ChannelMap;

        public:
            PollPoller(EventLoop *loop);
            virtual ~PollPoller();

            virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels);
            virtual void updateChannel(Channel *channel);
            virtual void removeChannel(Channel *channel);

        private:
            void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

            PollFdList pollfds_;
            ChannelMap channels_;
        };

    } // namespace net
} // namespace muduo
#endif // MUDUO_NET_POLLER_POLLPOLLER_H
