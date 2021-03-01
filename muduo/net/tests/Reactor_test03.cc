#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Channel.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>

#include <boost/bind.hpp>
#include <sys/timerfd.h>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;

class TestClient
{
private:
    EventLoop *_ploop;
    TcpClient _client;

    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->name() << " server addr: " << conn->peerAddress().toIpPort() << endl;
        }
        else
        {
            cout << conn->name() << " close connection" << endl;
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *pbuf, Timestamp time)
    {
        cout << "Rece msg: " << pbuf->retrieveAllAsString() << " at " << time.toFormattedString() << endl;
    }

public:
    TestClient(EventLoop *ploop, InetAddress addr) : _ploop(ploop), _client(ploop, addr, "TestClient")
    {
        _client.setConnectionCallback(boost::bind(&TestClient::onConnection, this, _1));
        _client.setMessageCallback(boost::bind(&TestClient::onMessage, this, _1, _2, _3));
    }

    void connect()
    {
        _client.connect();
    }
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);

    TestClient client(&loop, addr);
    client.connect();

    loop.loop();
}
