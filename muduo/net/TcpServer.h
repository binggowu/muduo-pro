#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <map>

#include <muduo/base/Types.h>
#include <muduo/net/TcpConnection.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
    namespace net
    {
        class Acceptor;
        class EventLoop;
        class EventLoopThreadPool;

        class TcpServer : boost::noncopyable
        {
        public:
            typedef boost::function<void(EventLoop *)> ThreadInitCallback;
            typedef std::map<string, TcpConnectionPtr> ConnectionMap;

            TcpServer(EventLoop *loop,
                      const InetAddress &listenAddr,
                      const string &nameArg);
            ~TcpServer();

            // 设置线程池大小.
            // 参数: umThreads
            //     0: 没有线程池, 所有连接的loop都使用 baseloop_.
            //     >0: 线程池大小.
            void setThreadNum(int numThreads);

            void setThreadInitCallback(const ThreadInitCallback &cb)
            {
                threadInitCallback_ = cb;
            }

            /// Starts the server if it's not listenning.
            ///
            /// It's harmless to call it multiple times.
            /// Thread safe.
            void start();

            // 设置 新连接建立时 的回调函数 Not thread safe.
            void setConnectionCallback(const ConnectionCallback &cb)
            {
                connectionCallback_ = cb;
            }

            // 设置 客户端发送的消息到来时 的回调函数 Not thread safe.
            void setMessageCallback(const MessageCallback &cb)
            {
                messageCallback_ = cb;
            }

            // 设置写回调函数. Not thread safe.
            void setWriteCompleteCallback(const WriteCompleteCallback &cb)
            {
                writeCompleteCallback_ = cb;
            }

            const string &hostport() const { return hostport_; }
            const string &name() const { return name_; }

        private:
            void newConnection(int sockfd, const InetAddress &peerAddr);
            void removeConnection(const TcpConnectionPtr &conn);
            void removeConnectionInLoop(const TcpConnectionPtr &conn);

            EventLoop *loop_; // baseloop_, 当有新连接到来时, 使用的是线程池(threadPool_)中的loop, 不是这个loop

            const string hostport_; // IP:port 字符串
            const string name_;     // 服务名

            boost::scoped_ptr<Acceptor> acceptor_;

            boost::scoped_ptr<EventLoopThreadPool> threadPool_; // 线程池

            ConnectionCallback connectionCallback_;       // 连接到来 和 连接关闭 时的回调函数
            MessageCallback messageCallback_;             // 消息到来回调函数
            WriteCompleteCallback writeCompleteCallback_; // 消息发送完毕
            ThreadInitCallback threadInitCallback_;       // TcpServer::removeConnection

            bool started_;
            int nextConnId_;            // 下一个连接ID
            ConnectionMap connections_; // TcpConnection列表
        };

    } // namespace net
} // namespace muduo

#endif
