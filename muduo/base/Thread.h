#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    class Thread : boost::noncopyable
    {
    public:
        typedef boost::function<void()> ThreadFunc;

        explicit Thread(const ThreadFunc &, const string &name = string());
        ~Thread();

        void start(); // 启动线程, 即调用 pthread_create()
        int join();   // 调用 pthread_join()

        bool started() const { return started_; }
        pid_t tid() const { return tid_; }
        const string &name() const { return name_; }

        static int numCreated() { return numCreated_.get(); }

    private:
        static void *startThread(void *thread); // 线程入口函数
        void runInThread();                     // 在 startThread()中被调用

        bool started_; // 线程是否开始执行
        pthread_t pthreadId_;
        pid_t tid_;
        ThreadFunc func_; // 在 runInThread()中被调用, 也是真正工作的函数
        string name_;

        static AtomicInt32 numCreated_; // 都构造函数中+1, 即每创建一个该Thread对象, 就+1.
    };

} // namespace muduo

#endif
