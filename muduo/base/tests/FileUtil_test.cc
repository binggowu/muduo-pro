#include <muduo/base/FileUtil.h>

#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace muduo;

int main()
{
    string result;
    int64_t size = 0;
    int err = FileUtil::readFile("/proc/self", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 21 0 0, 21: Is a directory

    err = FileUtil::readFile("/proc/self/cmdline", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 0 16 0

    err = FileUtil::readFile("/dev/null", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 0 0 0

    err = FileUtil::readFile("/dev/zero", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 0 1024 0

    err = FileUtil::readFile("/dev/zero", 102400, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 0 102400 0

    err = FileUtil::readFile("/notexist", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size); // 2 1024 0, 2: No such file or directory
}
