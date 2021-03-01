#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{
    class ThreadPool : boost::noncopyable
    {
    public:
        typedef boost::function<void()> Task;

        explicit ThreadPool(const string &name = string());
        ~ThreadPool();

        void start(int numThreads);
        void stop();

        void run(const Task &f);

    private:
        void runInThread(); // 线程队列中的线程要执行的函数
        Task take();        // 获取任务队列中的任务

        MutexLock mutex_;
        Condition cond_;
        string name_;
        boost::ptr_vector<muduo::Thread> threads_;
        std::deque<Task> queue_;
        bool running_;
    };

} // namespace muduo

#endif
