#include <muduo/base/Timestamp.h>
#include <vector>
#include <stdio.h>

using muduo::Timestamp;

void passByConstReference(const Timestamp &x)
{
    printf("%s\n", x.toString().c_str());
}

void passByValue(Timestamp x)
{
    printf("%s\n", x.toString().c_str());
}

// 插入1000'000个对象, 每个对象创建的时间差
void benchmark()
{
    // const常量前面增加一个k, google的编码规范.
    const int kNumber = 1000 * 1000;

    std::vector<Timestamp> stamps;
    stamps.reserve(kNumber);
    for (int i = 0; i < kNumber; ++i)
    {
        stamps.push_back(Timestamp::now());
    }
    printf("%s\n", stamps.front().toString().c_str());
    printf("%s\n", stamps.back().toString().c_str()); // .back()返回最后一个元素, 注意和.end()的区别.
    printf("%f\n", timeDifference(stamps.back(), stamps.front()));

    int increments[100] = {0};
    int64_t start = stamps.front().microSecondsSinceEpoch();
    for (int i = 1; i < kNumber; ++i)
    {
        int64_t next = stamps[i].microSecondsSinceEpoch();
        int64_t inc = next - start;
        start = next;

        if (inc < 0)
        {
            printf("reverse!\n"); // 时间差小于0是不可能的, 除非出现了bug
        }
        else if (inc < 100)
        {
            ++increments[inc];
        }
        else
        {
            printf("big gap %d\n", static_cast<int>(inc));
        }
    }

    for (int i = 0; i < 100; ++i)
    {
        printf("%2d: %d\n", i, increments[i]);
    }
}

int main()
{
    Timestamp tt(Timestamp::now());
    printf("%s\n", tt.toString().c_str());
    passByValue(tt);
    passByConstReference(tt);
    printf("----------- benchmark() ------------\n");
    benchmark();
}
 