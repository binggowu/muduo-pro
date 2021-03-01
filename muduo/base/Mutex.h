#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
    // 封装mutex的 创建 和 销毁.
    class MutexLock : boost::noncopyable
    {
    public:
        MutexLock() : holder_(0)
        {
            int ret = pthread_mutex_init(&mutex_, NULL);
            assert(ret == 0);
            (void)ret;
        }

        ~MutexLock()
        {
            assert(holder_ == 0);
            int ret = pthread_mutex_destroy(&mutex_);
            assert(ret == 0);
            (void)ret;
        }

        bool isLockedByThisThread()
        {
            return holder_ == CurrentThread::tid();
        }

        void assertLocked()
        {
            assert(isLockedByThisThread());
        }

        void lock() // 仅仅供MutexLockGuard使用, 用户不要使用.
        {
            pthread_mutex_lock(&mutex_);
            holder_ = CurrentThread::tid();
        }

        void unlock()
        {
            holder_ = 0;
            pthread_mutex_unlock(&mutex_);
        }

        pthread_mutex_t *getPthreadMutex() // 仅仅供Condition使用, 用户不要使用.
        {
            return &mutex_;
        }

    private:
        pthread_mutex_t mutex_;
        pid_t holder_; // 线程tid, 当前这个mutex被哪个线程所拥有, 0表示没有被任何线程所拥有
    };

    // ------------------------------------------
    // MutexLockGuard
    // ------------------------------------------

    // 封装mutex的 加锁 和 解锁.
    class MutexLockGuard : boost::noncopyable
    {
    public:
        explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~MutexLockGuard()
        {
            mutex_.unlock();
        }

    private:
        MutexLock &mutex_;
    };

} // namespace muduo

#define MutexLockGuard(x) error "Missing guard object name"
// 防止出现: MutexLockGuard(mutex_)这种错误, 忘了写变量名字, 产生一个临时变量后立马销毁.

#endif // MUDUO_BASE_MUTEX_H
