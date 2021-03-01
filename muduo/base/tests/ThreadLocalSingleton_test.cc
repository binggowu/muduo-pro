#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

class Test : boost::noncopyable
{
public:
    Test()
    {
        printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
    }

    ~Test()
    {
        printf("tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
    }

    const std::string &name() const { return name_; }
    void setName(const std::string &n) { name_ = n; }

private:
    std::string name_;
};

void threadFunc(const char *changeTo)
{
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

    muduo::ThreadLocalSingleton<Test>::instance().setName(changeTo);

    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

    // no need to manually delete it
    // muduo::ThreadLocalSingleton<Test>::destroy();
}

int main()
{
    printf("main tid: %d\n", muduo::CurrentThread::tid());
    muduo::ThreadLocalSingleton<Test>::instance().setName("main one");
    muduo::Thread t1(boost::bind(threadFunc, "thread1"));
    muduo::Thread t2(boost::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
    t2.join();

    pthread_exit(0);

    // 输出:
    // tid=12094, constructing 0x55e352988e70

    // tid=12095, constructing 0x7fc9fc000b20
    // tid=12095, 0x7fc9fc000b20 name=
    // tid=12095, 0x7fc9fc000b20 name=thread1
    // tid=12095, destructing 0x7fc9fc000b20 thread1

    // tid=12096, constructing 0x7fc9f4000b20
    // tid=12096, 0x7fc9f4000b20 name=
    // tid=12096, 0x7fc9f4000b20 name=thread2
    // tid=12096, destructing 0x7fc9f4000b20 thread2

    // tid=12094, 0x55e352988e70 name=main one
    // tid=12094, destructing 0x55e352988e70 main one
}
