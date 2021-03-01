#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>
#include <errno.h>
#include <fcntl.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);

    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

// TcpServer::start()中调用
void Acceptor::listen()
{
    loop_->assertInLoopThread();

    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

// (1) 调用 accept 来接受新连接
// (2) 调用用户回调 newConnectionCallback_
void Acceptor::handleRead()
{
    loop_->assertInLoopThread();

    InetAddress peerAddr(0);

    // 这里直接把cfd传递给cb, 不够好. 优化思路: 可以先创建Socket对象, 再用move把Socket给cb, 确保资源的安全释放.
    // 这里没有考虑fd消耗完的情况.
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        // 当服务器fd不够用了, 此时客户端发起连接, 对于客户端来说, 是会被立刻关闭连接, 而不是卡着等待服务器.
        if (errno == EMFILE) // fd不够用了
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
