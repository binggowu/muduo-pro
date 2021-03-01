#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    template <typename T>
    class ThreadLocal : boost::noncopyable
    {
    public:
        ThreadLocal()
        {
            pthread_key_create(&pkey_, &ThreadLocal::destructor); // 此处注册了 销毁key对应数据的函数
        }

        ~ThreadLocal()
        {
            pthread_key_delete(pkey_); // 只是销毁key, 并没有销毁key对应的实际数据.
        }

        T &value()
        {
            T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
            if (!perThreadValue)
            {
                T *newObj = new T();
                pthread_setspecific(pkey_, newObj);
                perThreadValue = newObj;
            }
            return *perThreadValue;
        }

    private:
        static void destructor(void *x)
        {
            T *obj = static_cast<T *>(x);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            delete obj;
        }

    private:
        pthread_key_t pkey_;
    };

} // namespace muduo

#endif

