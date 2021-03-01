#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/net/TcpConnection.h>

namespace muduo
{
    namespace net
    {
        class Connector;

        typedef boost::shared_ptr<Connector> ConnectorPtr;

        class TcpClient : boost::noncopyable
        {
        public:
            TcpClient(EventLoop *loop, const InetAddress &serverAddr, const string &name);
            ~TcpClient(); // force out-line dtor, for scoped_ptr members.

            void connect();
            void disconnect();
            void stop();

            TcpConnectionPtr connection() const
            {
                MutexLockGuard lock(mutex_);
                return connection_;
            }

            bool retry() const;

            void enableRetry() { retry_ = true; }

            /// Not thread safe.
            void setConnectionCallback(const ConnectionCallback &cb)
            {
                connectionCallback_ = cb;
            }

            /// Not thread safe.
            void setMessageCallback(const MessageCallback &cb)
            {
                messageCallback_ = cb;
            }

            /// Not thread safe.
            void setWriteCompleteCallback(const WriteCompleteCallback &cb)
            {
                writeCompleteCallback_ = cb;
            }

        private:
            /// Not thread safe, but in loop
            void newConnection(int sockfd);

            /// Not thread safe, but in loop
            void removeConnection(const TcpConnectionPtr &conn);

            EventLoop *loop_;
            ConnectorPtr connector_; // 用于主动发起连接, 类似TcpServer的Acceptor.

            ConnectionCallback connectionCallback_;       // 连接建立 回调函数
            MessageCallback messageCallback_;             // 消息到来 回调函数
            WriteCompleteCallback writeCompleteCallback_; // 数据发送完毕 回调函数

            bool retry_; // 指连接建立之后, 又断开的时候, 是否重连
            bool connect_;

            // always in loop thread
            const string name_; // name_ + nextConnId_ 用于标识一个连接
            int nextConnId_;    // name_ + nextConnId_ 用于标识一个连接

            mutable MutexLock mutex_;
            TcpConnectionPtr connection_; // Connector_ 连接成功以后, 得到一个 TcpConnection. 不同于TcpServer管理一堆TcpConnection, TcpClient只管理一个TcpConnection.
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TCPCLIENT_H
