#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/SocketsOps.h>

#include <iostream>
using namespace std;

void coConnection(int cfd, const muduo::net::InetAddress &addr)
{
    cout << "client: " << addr.toIpPort() << endl;
    char str[] = "hello world";
    muduo::net::sockets::write(cfd, str, sizeof(str));
    ::close(cfd);
}

int main()
{
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr(8000);
    muduo::net::Acceptor acceptor(&loop, addr);
    acceptor.setNewConnectionCallback(coConnection);
    acceptor.listen();

    loop.loop();
}
