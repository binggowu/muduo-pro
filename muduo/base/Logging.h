#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include <muduo/base/LogStream.h>
#include <muduo/base/Timestamp.h>

namespace muduo
{
    class Logger
    {
    public:
        enum LogLevel
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };

        // -------------------------------------
        // SourceFile
        // -------------------------------------

        // compile time calculation of basename of source file
        class SourceFile
        {
        public:
            template <int N>
            inline SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1)
            {
                const char *slash = strrchr(data_, '/'); // builtin function
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            explicit SourceFile(const char *filename) : data_(filename)
            {
                // C 库函数 char *strrchr(const char *str, int c), 在参数 str 所指向的字符串中搜索最后一次出现字符 c 的位置.
                const char *slash = strrchr(filename, '/');
                if (slash)
                {
                    data_ = slash + 1; // filename: "/tmp/muduo.log", slash: "/muduo.log", data_: "muduo.log"
                }
                size_ = static_cast<int>(strlen(data_));
            }

            const char *data_; // 保存filename中的文件名, 不是整个路径.
            int size_;         // data_的长度, 不包括'\0'
        };

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream &stream() { return impl_.stream_; }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const char *msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);

    private:
        // -------------------------------------
        // Impl
        // -------------------------------------
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;

            Impl(LogLevel level, int old_errno, const SourceFile &file, int line);

            // 格式化时间time_, 并将结果输出到stream_
            void formatTime();

            void finish();

            Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;
        };

        Impl impl_;
    };

    extern Logger::LogLevel g_logLevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }

#define LOG_TRACE                                          \
    if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                          \
    if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                          \
    if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
    muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()  // 等价于 LOG_ERROR
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream() // 等价于 LOG_FATAL

    const char *strerror_tl(int savedErrno);

    // Taken from glog/logging.h
    //
    // Check that the input is non NULL.  This very useful in constructor
    // initializer lists.

#define CHECK_NOTNULL(val) \
    ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
    {
        if (ptr == NULL)
        {
            Logger(file, line, Logger::FATAL).stream() << names;
        }
        return ptr;
    }

} // namespace muduo

#endif // MUDUO_BASE_LOGGING_H
