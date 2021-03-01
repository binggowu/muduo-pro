#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{
    template <typename T>
    class Singleton : boost::noncopyable
    {
    public:
        static T &instance()
        {
            pthread_once(&ponce_, &Singleton::init); // 原子操作, 保证 Singleton::init 只被执行一次.
            return *value_;
        }

    private:
        Singleton();
        ~Singleton();

        static void init()
        {
            value_ = new T();
            ::atexit(destroy); // atexit注册一个函数, 在main()结束后被调用.
        }

        static void destroy()
        {
            // 对不完整类型T, 可以执行T *p; delete p; 编译时只会报warning, 不会报错.
            // 此处, 如果T为不完整类型, 就定义了char arr[-1], 长度为-1的数组, 在编译时就报错.
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            delete value_;
        }

    private:
        static pthread_once_t ponce_;
        static T *value_;
    };

    template <typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template <typename T>
    T *Singleton<T>::value_ = NULL;

} // namespace muduo

#endif
