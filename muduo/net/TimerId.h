#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include <muduo/base/copyable.h>

namespace muduo
{
    namespace net
    {
        class Timer;

        class TimerId : public muduo::copyable
        {
        public:
            TimerId()
                : timer_(NULL),
                  sequence_(0)
            {
            }

            TimerId(Timer *timer, int64_t seq)
                : timer_(timer),
                  sequence_(seq)
            {
            }

            friend class TimerQueue;

        private:
            Timer *timer_;     // 不负责Timer的声明周期, timer_也可能是失效的.
            int64_t sequence_; // 仅仅包含*timer_是不够的, 这样无法区分地址相同的前后两个Timer对象, 因此每一个Timer对象都由一个全局的sequence_序列号.
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TIMERID_H
