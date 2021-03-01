#include <muduo/base/Logging.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const string &nameArg)
    : loop_(CHECK_NOTNULL(loop)),
      hostport_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr)),
      threadPool_(new EventLoopThreadPool(loop)), // loop就是mainReadtor
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      started_(false),
      nextConnId_(1)
{
    // Acceptor::handleRead()中会回调用TcpServer::newConnection. _1: cfd, _2: 客户端的地址
    acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (auto it = connections_.begin(); it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(
            boost::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();
    }
}

// 设置线程池大小.
// 参数: umThreads
//     0: 没有线程池, 所有连接的loop都使用 baseloop_.
//     >0: 线程池大小.
void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

// 开始listen事件
void TcpServer::start()
{
    if (!started_)
    {
        started_ = true;
        threadPool_->start(threadInitCallback_);
    }

    if (!acceptor_->listenning())
    {
        // get_pointer返回原生指针
        loop_->runInLoop(boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

// Acceptor中的newConnectionCallback_, 创建一个TcpConnection对象, 并设置4个回调函数.
// 参数sockfd: cfd
// 参数peerAddr: 客户端的地址
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    loop_->assertInLoopThread();

    EventLoop *ioLoop = threadPool_->getNextLoop(); // 从线程池中选择一个线程

    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    // 创建TcpConnection所属的loop是线程池中的loop, 不是成员变量loop_
    // 核心, 让TcpConnection和EventLoopThreadPool中的loop_相关联.
    TcpConnectionPtr conn(new TcpConnection(ioLoop, // 测试一下, 如果传入loop_会怎么样.
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    // 此时 conn 的引用技术为1
    // LOG_TRACE << "[1] use_count: " << conn.use_count();
    connections_[connName] = conn;
    // 此时 conn的引用技术为2, 上面加入 map了拷贝.
    // LOG_TRACE << "[2] use_count: " << conn.use_count();

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe

    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn)); // 让 TcpConnection所属的 loop调用连接函数(TcpConnection::connectEstablished).
}

// 从connections_中移除conn, 在loop_中这注册了removeConnectionInLoop(). 线程安全
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 仅仅被上面的TcpServer::removeConnection()调用
// (1) 从connections_中移除conn -> (2) 把conn的connectDestroyed()放进EventLoop的functors中.
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();

    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}
