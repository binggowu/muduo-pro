/* #include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>
#include <stdio.h>

using namespace muduo;
using namespace std;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10 * 1000 * 1000;

void threadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int main()
{
    const int kMaxThreads = 8;
    g_vec.reserve(kMaxThreads * kCount);

    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i);
    }
    printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

    start = Timestamp::now();
    threadFunc();
    printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

    for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)
    {
        boost::ptr_vector<Thread> threads;
        g_vec.clear();
        start = Timestamp::now();
        for (int i = 0; i < nthreads; ++i)
        {
            threads.push_back(new Thread(&threadFunc));
            threads.back().start();
        }
        for (int i = 0; i < nthreads; ++i)
        {
            threads[i].join();
        }
        printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
    }
}
 */

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include "muduo/base/Thread.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>
#include <vector>
using namespace std;
using namespace muduo;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10'000'000;

void ThreadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int main()
{
    const int kThreads = 8;
    g_vec.reserve(kCount * kThreads);

    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i);
    }
    printf("without lock: %f\n", timeDifference(Timestamp::now(), start));

    g_vec.clear();
    start = Timestamp::now();
    ThreadFunc();
    printf("with lock: %f\n", timeDifference(Timestamp::now(), start));

    for (int i = 1; i < kThreads; ++i)
    {
        boost::ptr_vector<Thread> threads;
        g_vec.clear();
        start = Timestamp::now();
        for (int j = 0; j < i; ++j)
        {
            threads.push_back(new Thread(&ThreadFunc));
            threads.back().start();
        }
        for (int j = 0; j < i; ++j)
        {
            threads[j].join();
        }
        printf("thread num: %d, time: %f\n", i, timeDifference(Timestamp::now(), start));
    }
}
