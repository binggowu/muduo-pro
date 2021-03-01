#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
    namespace net
    {
        class EventLoop;

        class Channel : boost::noncopyable
        {
        public:
            typedef boost::function<void()> EventCallback;              // write/error/close回调函数
            typedef boost::function<void(Timestamp)> ReadEventCallback; // read回调函数

            Channel(EventLoop *loop, int fd);
            ~Channel();

            /// Tie this channel to the owner object managed by shared_ptr,
            /// prevent the owner object being destroyed in handleEvent.
            void tie(const boost::shared_ptr<void> &);

            int fd() const { return fd_; }

            // ---------
            // event相关
            // ---------

            int events() const { return events_; }
            void set_revents(int revt) { revents_ = revt; }

            // 判断当前Channel有没有监听事件, 用于EPollPoller::updateChannel()中选择EPOLL_CTL_MOD/DEL.
            bool isNoneEvent() const { return events_ == kNoneEvent; }

            // 设置读事件: POLLIN | POLLPRI, 并在epoll关注该channel
            void enableReading()
            {
                events_ |= kReadEvent;
                update();
            }

            // void disableReading() { events_ &= ~kReadEvent; update(); }
            void enableWriting()
            {
                events_ |= kWriteEvent;
                update();
            }

            void disableWriting()
            {
                events_ &= ~kWriteEvent;
                update();
            }

            void disableAll()
            {
                events_ = kNoneEvent;
                update();
            }

            bool isWriting() const { return events_ & kWriteEvent; }

            // ---------
            // 回调函数相关
            // ---------

            void handleEvent(Timestamp receiveTime);

            void setReadCallback(const ReadEventCallback &cb)
            {
                readCallback_ = cb;
            }

            void setWriteCallback(const EventCallback &cb)
            {
                writeCallback_ = cb;
            }

            void setCloseCallback(const EventCallback &cb)
            {
                closeCallback_ = cb;
            }

            void setErrorCallback(const EventCallback &cb)
            {
                errorCallback_ = cb;
            }

            // ---------
            // Poller相关
            // ---------

            int index() { return index_; }
            void set_index(int idx) { index_ = idx; }

            // for debug
            string reventsToString() const;

            void doNotLogHup() { logHup_ = false; }

            EventLoop *ownerLoop() { return loop_; }
            void remove();

        private:
            void update();
            void handleEventWithGuard(Timestamp receiveTime);

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop *loop_; // 所属的EventLoop
            const int fd_;    // 不负责关闭该fd,  有EventLoop::wakeupfd_
            int events_;      // 关注的IO事件, 由用户设置.
            int revents_;     // 当前活跃的IO事件, 由EventLoop/Poller设置, 即poll/epoll返回的IO事件.
            int index_;       // 表示在poll的事件数组中序号, 见EpollPoller.cc的25-30行
            bool logHup_;     // for POLLHUP

            boost::weak_ptr<void> tie_; // 把 TcpConnection 对象的 this 赋值给 tie_, 见 TcpConnection::connectEstablished(). void可以接受任意类型.
            bool tied_;                 // 这个没有用到.

            bool eventHandling_; // 是否正在处理事件, 即是否正在 handleEvent()函数中.

            /* 
            Channel 的 ReadCallback
            • TimerQueue 用它来读 timerfd.
            • EventLoop 用它来读 eventfd.
            • Acceptor 用它来读 lfd.
            • TcpConnection用它来读cfd. 
            */
            ReadEventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_; //
            EventCallback errorCallback_;
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_CHANNEL_H
