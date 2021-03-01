#include <muduo/base/Timestamp.h>
#include <sys/time.h>
#include <stdio.h>
#define __STDC_FORMAT_MACROS  // 本机测试, 可以不需要定义这个宏, inttypes.h源码也没有这个条件了.
#include <inttypes.h>  // PRId64
#undef __STDC_FORMAT_MACROS

#include <boost/static_assert.hpp>

using namespace muduo;

// BOOST_STATIC_ASSERT: 编译时断言
// assert(): 运行时断言
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));

Timestamp::Timestamp(int64_t microseconds) : microSecondsSinceEpoch_(microseconds)
{
}

string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

    // PRId64的含义
    //   seconds, microseconds的类型是int64_t, 
    //   在64位系统中, 用long int表示64位整数, 打印 printf("%ld", val); 
    //   在32位系统中, 用long long int表示64位整数, 打印 printf("%lld", val);
    //   跨平台打印: printf("%" PRId64, val);
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

string Timestamp::toFormattedString() const
{
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);

    // 总秒数 -> 年/月/日/时/分/秒
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);  // gmtime_r() 是线程安全的, 而 gmtime()不是线程安全的.

    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);

    return buf;
}

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL); // 第二个参数NULL: 不需要设定时区
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec); // tv.tv_usec就是微秒
}

Timestamp Timestamp::invalid()
{
    return Timestamp();
}
