#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
    // 倒计时, 一种同步手段. 主要用途:
    // 1) 主线程创建多个子线程, 等待子线程完成一定任务后, 主线程才开始执行. 如主线程等待子线程完成初始化工作.
    // 2) 主线程创建多个子线程, 子线程先等待主线程完成一定任务之后才开始执行. 如子线程等待主线程发起"起跑"命令.
    class CountDownLatch : boost::noncopyable
    {
    public:
        explicit CountDownLatch(int count);

        void wait();

        void countDown();

        int getCount() const;

    private:
        // 注意: Condition依赖MutexLock, 所以MutexLock必须在Condition前面.

        mutable MutexLock mutex_; // mutex_的lock/unlock会改变其内部状态, 为防止程序员声明const的CountDownLatch, 所以要用mutable以防万一.
        Condition condition_;
        int count_;
    };

} // namespace muduo
#endif // MUDUO_BASE_COUNTDOWNLATCH_H
