#include <muduo/base/CountDownLatch.h>

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    : mutex_(),
      condition_(mutex_), // 注意: Condition依赖MutexLock, 所以MutexLock必须在Condition前面.
      count_(count)
{
}

// count_大于0就阻塞等待
void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
    {
        condition_.wait();
    }
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
    {
        condition_.notifyAll();
    }
}

int CountDownLatch::getCount() const
{
    MutexLockGuard lock(mutex_); // 可能多个线程都访问count_, 需要保护一下
    return count_;
}
