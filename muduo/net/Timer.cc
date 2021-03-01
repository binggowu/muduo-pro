#include <muduo/net/Timer.h>

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

// 重新计算下一个超时时刻, expiration_ = n0w + interval_
void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}
