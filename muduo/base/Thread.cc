#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
    namespace CurrentThread
    {
        __thread int t_cachedTid = 0;  // 线程tid (真实pid), tid最小是1, 所以0可以作为无效的tid.
        __thread char t_tidString[32]; // tid的字符串表示
        __thread const char *t_threadName = "unknown";

        const bool sameType = boost::is_same<int, pid_t>::value; // 判断pid_t是不是int类型
        BOOST_STATIC_ASSERT(sameType);

    } // namespace CurrentThread

    namespace detail
    {
        pid_t gettid()
        {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        // 在调用fork()后, 子进程会调用这个函数,
        void afterFork()
        {
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main"; // 子线程会成为新fork()出来进程的主线程
            CurrentThread::tid();
        }

        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer()
            {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(NULL, NULL, &afterFork);

            }
        };

        ThreadNameInitializer init; // 这个对象的构造先于main()
        // 如果在子线程中调用fork(), 那么该子线程会成为新fork()出来进程的主线程, 我们希望改变这个线程的 t_cachedTid 和 t_threadName.

    } // namespace detail
} // namespace muduo

using namespace muduo;

// -------------------------------------------------------------------------
// CurrentThread
// -------------------------------------------------------------------------

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        int n = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
        assert(n == 6);
        (void)n; // 防止n没有被使用, 编译时发出警告.
    }
}

// 主线程的pid和tid是一样的.
bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

// -------------------------------------------------------------------------
// Thread
// -------------------------------------------------------------------------

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(n)
{
    numCreated_.increment();
}

Thread::~Thread()
{
}

// 启动线程, 即调用 pthread_create()
void Thread::start()
{
    assert(!started_);

    started_ = true;
    errno = pthread_create(&pthreadId_, NULL, &startThread, this);
    if (errno != 0)
    {
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int Thread::join()
{
    assert(started_);
    return pthread_join(pthreadId_, NULL);
}

void *Thread::startThread(void *obj)
{
    Thread *thread = static_cast<Thread *>(obj);
    thread->runInThread();
    return NULL;
}

void Thread::runInThread()
{
    tid_ = CurrentThread::tid();
    muduo::CurrentThread::t_threadName = name_.c_str();

    try
    {
        func_();
        muduo::CurrentThread::t_threadName = "finished";
    }
    catch (const Exception &ex)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception &ex)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw; // rethrow
    }
}
