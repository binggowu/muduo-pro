#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class InetAddress;

        // 内部类, 作为TcpServer的成员, 生命周期由TcpServer管理.
        class Acceptor : boost::noncopyable
        {
        public:
            typedef boost::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

            Acceptor(EventLoop *loop, const InetAddress &listenAddr);
            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback &cb)
            {
                newConnectionCallback_ = cb;
            }

            bool listenning() const { return listenning_; }
            void listen();

        private:
            void handleRead();

            EventLoop *loop_;
            Socket acceptSocket_;   // lfd
            Channel acceptChannel_; // 关注lfd的readable事件, 并回调handleRead()
            // 在构造函数Acceptor()中set cb, 在listen()中enable cb.

            NewConnectionCallback newConnectionCallback_; // 在TcpServer::TcpServer()中设置
            bool listenning_;
            int idleFd_; // 故意占用一个fd.
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_ACCEPTOR_H
