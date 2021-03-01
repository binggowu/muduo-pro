#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/Mutex.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/any.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace muduo
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Socket;

        class TcpConnection : boost::noncopyable,
                              public boost::enable_shared_from_this<TcpConnection>
        {
        public:
            // 每一个TcpConnection都有自己的名字(TcpServer的name+IP:port+index), 这个名字是由其所属的TcpServer在创建TcpConnection对象时生成的, 名字是ConnectionMap的key.
            TcpConnection(EventLoop *loop,
                          const string &name,
                          int sockfd,
                          const InetAddress &localAddr,
                          const InetAddress &peerAddr);
            ~TcpConnection();

            void setTcpNoDelay(bool on);

            // 在非阻塞网络编程中, 发送消息通常是由网络库完成的, 用户不会直接调用write或send系统调用.
            /* 
            TcpConnection::send()
            • send()的返回类型是void, 意味着用户不必关心send()时成功发送了多少字节, muduo库会保证把数据发送给对方.
            • send()是非阻塞的, 即使当前TCP的发送窗口满了, 也绝对不会阻塞当前线程.
            • send()是线程安全的(sendInLoop), 多个线程可以同时调用send(), 消息之间不会混乱. 
            */
            void send(const void *message, size_t len);
            void send(const StringPiece &message);
            void send(Buffer *message); // this one will swap data
            // void send(string&& message); // C++11
            // void send(Buffer&& message); // C++11

            // -----------
            // context_ 相关
            // -----------

            void setContext(const boost::any &context) { context_ = context; }
            const boost::any &getContext() const { return context_; }
            boost::any *getMutableContext() { return &context_; }

            // -----------
            // cb 相关
            // -----------

            void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
            void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
            void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
            {
                highWaterMarkCallback_ = cb;
                highWaterMark_ = highWaterMark;
            }
            void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

            // 关闭写端, 不是线程安全.
            void shutdown();

            // called when TcpServer accepts a new connection
            void connectEstablished();

            void connectDestroyed();

            EventLoop *getLoop() const { return loop_; }
            const string &name() const { return name_; }
            const InetAddress &localAddress() { return localAddr_; }
            const InetAddress &peerAddress() { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            Buffer *inputBuffer() { return &inputBuffer_; }

        private:
            // -------
            // 连接状态
            // -------
            enum StateE
            {
                kDisconnected,
                kConnecting,
                kConnected,
                kDisconnecting
            };
            StateE state_; // FIXME: use atomic variable, 连接的状态
            void setState(StateE s) { state_ = s; }

            // ----------
            // channel相关
            // ----------

            void handleRead(Timestamp receiveTime);
            void handleWrite();
            void handleClose(); // 由Channel的CloseCallback调用.
            void handleError();
            boost::scoped_ptr<Channel> channel_;

            // ---------
            // socket相关
            // ---------

            boost::scoped_ptr<Socket> socket_;
            InetAddress localAddr_;
            InetAddress peerAddr_;

            void sendInLoop(const StringPiece &message);
            void sendInLoop(const void *message, size_t len);
            // void sendInLoop(string&& message);

            void shutdownInLoop();

            EventLoop *loop_; // 所属EventLoop
            string name_;

            // 这4个回调函数在创建 connection时被调用, 即TcpServer::newConnection().

            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            CloseCallback closeCallback_; // 给TcpServer和TcpClient使用的, 用于通知他们移除所有的TcpConnectionPtr, 如 TcpServer::removeConnection/TcpClient::removeConnection

            size_t highWaterMark_;                        // 高水位标
            WriteCompleteCallback writeCompleteCallback_; // sendInLoop()/handleWrite()中被调用. 数据发送完毕回调函数, 即所有的用户数据都已拷贝到内核缓冲区时回调该函数. outputBuffer_被清空也会回调该函数, 可以理解为 "低水位" 回调函数
            // 大流量: 不断生成数据, 然后conn->send(), 如果对象接受不及时, 受到通告滑动窗口的控制, 内核发送缓冲不足, 这个时候, 就会将用户数据添加到应用层发送缓冲区.

            HighWaterMarkCallback highWaterMarkCallback_; // "高水位" 标回调函数

            // input/output是针对程序员而言的, 对TcpConnection而言相反.
            // TcpConnection会从cfd读取数据, 然后写入inputBuffer_, 这一步是由Buffer::readfd()完成的, 程序员从inputBuffer_中读取数据.
            // 程序员应该在onMessage()完成对inputBuffer_的操作.
            Buffer inputBuffer_;

            Buffer outputBuffer_; // 程序员把数据写入outputBuffer, 这一步是由TcpConnection::send()完成的, TcpConnection从outputBuffer中读取数据并写入cfd.

            // 可变类型解决方案: 1) void*, 但不是类型安全的; 2) boost::any.
            // boost::any: 任意类型的安全存储, 以及安全取. 还可以这么使用: std::vector<boost::any>, 以存放任意类型数据.
            boost::any context_; // 绑定一个未知类型的上下文对象, 给上层应用预留一个成员.

            // FIXME: creationTime_, lastReceiveTime_, bytesReceived_, bytesSent_
        };

        typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TCPCONNECTION_H
