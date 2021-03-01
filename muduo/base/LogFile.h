#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
    class LogFile : boost::noncopyable
    {
    public:
        LogFile(const string &basename,
                size_t rollSize,
                bool threadSafe = true,
                int flushInterval = 3);
        ~LogFile();

        void append(const char *logline, int len);
        void flush();

    private:
        void append_unlocked(const char *logline, int len);

        static string getLogFileName(const string &basename, time_t *now);
        void rollFile(); // 滚动日志

        const string basename_;   // 只能是日志文件名, 不允许存在路径('/')
        const size_t rollSize_;   // 日志文件大小到达rollSize_就换一个新文件写
        const int flushInterval_; // 日志写入的时间间隔

        int count_; // 计数器, 和kCheckTimeRoll配合使用

        boost::scoped_ptr<MutexLock> mutex_;
        time_t startOfPeriod_; // 开始写日志的时间, 距离"零点"时间的秒数(按天对齐)
        time_t lastRoll_;      // 上一次滚动日志的时间
        time_t lastFlush_;     // 上一次日志写入的时间
        
        class File;
        boost::scoped_ptr<File> file_;

        const static int kCheckTimeRoll_ = 1024;
        const static int kRollPerSeconds_ = 60 * 60 * 24; // 一天
    };

} // namespace muduo

#endif // MUDUO_BASE_LOGFILE_H
