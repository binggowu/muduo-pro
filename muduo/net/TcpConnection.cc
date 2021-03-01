#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Socket.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

// 在TcpServer的构造函数中使用
void muduo::net::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
}

// 在TcpServer的构造函数中使用
void muduo::net::defaultMessageCallback(const TcpConnectionPtr &,
                                        Buffer *buf,
                                        Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(nameArg),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
    // channel可读事件到来的时候, 回调TcpConnection::handleRead, _1是事件发生时间
    channel_->setReadCallback(boost::bind(&TcpConnection::handleRead, this, _1));

    channel_->setWriteCallback(boost::bind(&TcpConnection::handleWrite, this));

    // 连接关闭, 回调TcpConnection::handleClose
    channel_->setCloseCallback(boost::bind(&TcpConnection::handleClose, this));

    // 发生错误, 回调TcpConnection::handleError
    channel_->setErrorCallback(boost::bind(&TcpConnection::handleError, this));

    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd();
}

void TcpConnection::send(const void *data, size_t len)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            string message(static_cast<const char *>(data), len);
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::send(const StringPiece &message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,                  // FIXME
                            message.as_string())); // 把message这块内存复制到IO线程, 有开销.
            // std::forward<string>(message)));
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this, // FIXME
                            buf->retrieveAllAsString()));
            // std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece &message)
{
    sendInLoop(message.data(), message.size());
}

// 先直接write, 如果内核缓冲区满了, 再将剩余部分写入用户缓冲区(outputBuffer_)并关注POLLOUT事件.
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    loop_->assertInLoopThread();
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    ssize_t nwrote = 0;
    size_t remaining = len;
    bool error = false;

    // channel_没有关注可写事件, 并且outputBuffer_没有数据, 直接write
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            // 写完了, 回调writeCompleteCallback_
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // 报错
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE) // FIXME: any others?
                {
                    error = true;
                }
            }
        }
    }

    // 还有未写完的数据, 说明内核发送缓冲区满了, 要将未写完的数据添加到outputBuffer_中.
    assert(remaining <= len);
    if (!error && remaining > 0)
    {
        LOG_TRACE << "I am going to write more data";
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果超过highWaterMark_(高水位标), 回调highWaterMarkCallback_
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 关注POLLOUT事件
        }
    }
}

// 1. outputBuffer_ 中没有数据等待发送, 直接关闭写端.
// 2. outputBuffer_ 中有数据等待发送, 则设置状态为 kDisconnecting, 并没有关闭. 当 outputBuffer_ 中的数据发送完毕, 就会调用shutdownInLoop()来关闭, 见 handleWrite().
// 3. 关闭写端后, 客户端read()就返回0, 客户端一般都会close(cfd), 此时服务端的cfd可读(POLLIN | POLLHUP), 就会触发channel的可读事件, 调用Channel的closeCallback_关闭连接. 见Channel::handleEventWithGuard().
void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();

    if (!channel_->isWriting()) // 正在发送数据, 即channel关注了write事件, output buffer中有数据没有发送完毕.
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

// 关注channel的写事件, 并执行用户的回调函数
// 调用: TcpServer/TcpClient的newConnection()
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);

    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // TcpConnection所对应的channel加入到Poller关注

    connectionCallback_(shared_from_this()); // connectionCallback_: 用户的回调函数
}

// 销毁一个TcpConnection最后要调用的函数.
// 在TcpServer::removeConnectionInLoop()中被调用
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();

    if (state_ == kConnected) // 不会成立, 因为在 handleClose()中已经设置为kDisconnected, 而该函数的触发肯定要调用 handleClose()的.
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }

    channel_->remove(); // // 从 loop_中移除该 channel
}

// 内部会检查read()的返回值, 并根据返回值分别调用messageCallback_(), handleClose(), handleError().
void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();

    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // shared_from_this(): 把this转化为share_prt
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose(); // 处理连接断开
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

// 内核发送缓冲区有空间了, 回调该函数
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();

    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) // outputBuffer_中数据全部发送完毕: 1) channel取消EPOLLOUT事件; 2) 调用writeCompleteCallback_.
            {
                channel_->disableWriting(); //  1) channel取消EPOLLOUT事件, 以免出现 busy loop
                if (writeCompleteCallback_)
                {
                    // 应用层发送缓冲区被清空, 就回调用writeCompleteCallback_
                    loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
                }

                if (state_ == kDisconnecting) // 在send()时调用过shutdown().
                {
                    shutdownInLoop();
                }
            }
            else // 还有数据没有发送完
            {
                LOG_TRACE << "I am going to write more data";
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

// 处理连接断开, 内部调用 closeCallbackk_
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);

    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this()); // TcpConnectionPtr guardThis(this); 不能这么用,
    connectionCallback_(guardThis);                 // 用户的回调函数

    closeCallback_(guardThis);
}

// 仅仅是记录日志
void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
