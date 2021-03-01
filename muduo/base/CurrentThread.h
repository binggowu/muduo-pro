#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo
{
    namespace CurrentThread
    {
        // 在Thread.cc中定义
        extern __thread int t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread const char *t_threadName;

        void cacheTid();

        inline int tid()
        {
            if (t_cachedTid == 0)
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char *tidString() // 调用该函数前, 必须已经调用过 tid(). 用于日志.
        {
            return t_tidString;
        }

        inline const char *name()
        {
            return t_threadName;
        }

        bool isMainThread();

    } // namespace CurrentThread
} // namespace muduo

#endif
