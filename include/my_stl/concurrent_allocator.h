#ifndef _CONCURRENT_ALLOCATOR_H_
#define _CONCURRENT_ALLOCATOR_H_

#include <mutex>
#include "alloc.h"

namespace MySTL
{
    /*
     **线程安全空间配置器
     **对 alloc 加互斥锁包装, 供多线程数据结构（如并发跳表）使用
     */

    class concurrent_allocator
    {
    private:
        inline static std::mutex mtx_;

    public:
        static void *allocate(size_t bytes)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return alloc<false, 0>::allocate(bytes);
        }
        static void deAllocate(size_t bytes, void *p)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            alloc<false, 0>::deAllocate(bytes, p);
        }
        static void *reAllocate(size_t oldSize, size_t newSize, void *p)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return alloc<false, 0>::reAllocate(oldSize, newSize, p);
        }
    };

} // namespace MySTL

#endif
