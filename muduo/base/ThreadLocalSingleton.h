#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
    template <typename T>
    class ThreadLocalSingleton : boost::noncopyable
    {
    public:
        // 不需要安装线程安全的方式去实现, 因为t_value_被__thread修饰的.
        static T &instance()
        {
            if (!t_value_)
            {
                t_value_ = new T();
                deleter_.set(t_value_);
            }
            return *t_value_;
        }

        static T *pointer()
        {
            return t_value_;
        }

    private:
        static void destructor(void *obj)
        {
            assert(obj == t_value_);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1]; // 不完全类型检查
            delete t_value_;
            t_value_ = 0;
        }

        // 该类的作用就是为了销毁 t_value_ 对象, 实现如下:
        // 1. 构造delete_对象时, 注册回调函数 ThreadLocalSingleton::destructor;
        // 2. 析构delete_对象时, 调用回调函数, 销毁t_value_对象.
        class Deleter
        {
        public:
            Deleter()
            {
                pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
            }

            ~Deleter()
            {
                pthread_key_delete(pkey_);
            }

            void set(T *newObj)
            {
                assert(pthread_getspecific(pkey_) == NULL);
                pthread_setspecific(pkey_, newObj);
            }

            pthread_key_t pkey_; // pthread_key_t: 线程本地存储机制
        };

        static __thread T *t_value_; // __thread: 线程本地存储机制, 被__thread修饰的变量, 每个线程都有一份.
        static Deleter deleter_;     // 用于销毁t_value_所指对象
    };

    template <typename T>
    __thread T *ThreadLocalSingleton<T>::t_value_ = 0;

    template <typename T>
    typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

} // namespace muduo

#endif
