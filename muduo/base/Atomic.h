#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace muduo
{
    namespace detail
    {
        template <typename T>
        class AtomicIntegerT : boost::noncopyable
        {
        public:
            AtomicIntegerT() : value_(0)
            {
            }

            // uncomment if you need copying and assignment
            //
            // AtomicIntegerT(const AtomicIntegerT& that)
            //   : value_(that.get())
            // {}
            //
            // AtomicIntegerT& operator=(const AtomicIntegerT& that)
            // {
            //   getAndSet(that.get());
            //   return *this;
            // }

            T get()
            {
                // type __sync_val_compare_and_swap(type *ptr, type oldval, type newval), 原子设置操作, 避免锁竞争(速度是线程锁的6～7倍).
                //   如果*ptr == oldval, 则 *ptr = newval, 返回*ptr.
                //   如果*ptr != oldval, 则直接返回*ptr.
                // 此处的含义就是获取value_, 只不过是原子操作.
                return __sync_val_compare_and_swap(&value_, 0, 0);
            }

            // 原子性的value_++
            T getAndAdd(T x)
            {
                // type __sync_fetch_and_add(type *ptr, type value), 原子自增操作, 先fetch然后add, 返回的是自加以前的值, 含义同i++.
                // __snyc_add_and_fetch, 含义同++i;
                return __sync_fetch_and_add(&value_, x);
            }

            // 原子性的++value_
            T addAndGet(T x)
            {
                // 等价于 __snyc_add_and_fetch(&value, x);
                return getAndAdd(x) + x;
            }

            T incrementAndGet()
            {
                return addAndGet(1);
            }

            T decrementAndGet()
            {
                return addAndGet(-1);
            }

            void add(T x)
            {
                getAndAdd(x);
            }

            void increment()
            {
                incrementAndGet();
            }

            void decrement()
            {
                decrementAndGet();
            }

            T getAndSet(T newValue)
            {
                // type __sync_lock_test_and_set(type *ptr, type value);, 将*ptr设为value并返回*ptr操作之前的值
                return __sync_lock_test_and_set(&value_, newValue);
            }

        private:
            // volatile声明变量: 要求系统总是从变量所在的内存位置读取数据, 而不是使用保存在寄存器中的备份, 哪怕刚刚上一条指令就是从内存中读取值到寄存器.
            // 多线程编程很重要, 即防止编译器对代码进行优化.
            volatile T value_;
        };

    } // namespace detail

    typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
    typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

} // namespace muduo

#endif // MUDUO_BASE_ATOMIC_H
