#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <cstdio>
#include <sys/timerfd.h>

muduo::net::EventLoop *g_pLoop;

void threadFunc(muduo::Timestamp time)
{
    g_pLoop->quit();
}

int main()
{
    muduo::net::EventLoop loop;
    g_pLoop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    muduo::net::Channel channel(g_pLoop, timerfd);
    channel.setReadCallback(threadFunc);
    channel.enableReading();

    itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, nullptr);

    g_pLoop->loop();
}
