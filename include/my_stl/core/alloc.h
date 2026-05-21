#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

namespace MySTL
{
    /*
     **空间配置器，以字节数为单位分配
     **内部使用
     */

    template <bool threads, int inst> // 避免单一定义规则
    class alloc
    {
    private:
        static constexpr size_t ALIGN = 8;
        static constexpr size_t MAXBYTES = 128;
        static constexpr size_t NFREELISTS = MAXBYTES / ALIGN;
        static constexpr size_t NOBJS = 20;

    private:
        // free-lists的节点构造, 零额外开销
        union obj
        {
            union obj *next;
            char clientData[1];
        };

        // 16个free-lists, 每个free-list管理一种区块大小
        static obj *freeList[NFREELISTS];

    private:
        // 将bytes上调至8的倍数
        static size_t FREELIST_INDEX(size_t bytes)
        {
            return (((bytes) + ALIGN - 1) / ALIGN - 1);
        }
        // 与-7进行位运算, 强制抹除低位的三个0, 保持为8的倍数
        static size_t ROUND_UP(size_t bytes)
        {
            return ((bytes + ALIGN - 1) & ~(ALIGN - 1));
        }

    private:
        // 内存池相关函数及其变量
        static void *refill(size_t bytes);
        static char *chunkAlloc(size_t size, size_t &nobjs);
        static char *startFree; // 内存池起始位置, 只在chunk_alloc()中变化
        static char *endFree;   // 与上同理
        static size_t heapSize;

    public:
        // 负责内存分配释放
        static void *allocate(size_t bytes);
        static void deAllocate(size_t bytes, void *p);
        static void *reAllocate(size_t oldSize, size_t newSize, void *p);
    };

    // 静态成员的初始化与定义
    template <bool threads, int inst>
    char *alloc<threads, inst>::startFree{nullptr};

    template <bool threads, int inst>
    char *alloc<threads, inst>::endFree{nullptr};

    template <bool threads, int inst>
    size_t alloc<threads, inst>::heapSize{0};

    // static constexpr 成员类外定义（C++14兼容；C++17起非必须）
    template <bool threads, int inst>
    constexpr size_t alloc<threads, inst>::ALIGN;
    template <bool threads, int inst>
    constexpr size_t alloc<threads, inst>::MAXBYTES;
    template <bool threads, int inst>
    constexpr size_t alloc<threads, inst>::NFREELISTS;
    template <bool threads, int inst>
    constexpr size_t alloc<threads, inst>::NOBJS;

    template <bool threads, int inst>
    typename alloc<threads, inst>::obj *alloc<threads, inst>::freeList[] = {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    // 内存分配实现
    template <bool threads, int inst>
    void *alloc<threads, inst>::allocate(size_t bytes)
    {
        // 由于没有一级配置器, 大于128直接malloc分配
        if (bytes > MAXBYTES)
        {
            return malloc(bytes);
        }

        size_t index = FREELIST_INDEX(bytes);
        obj *list = freeList[index]; // 摘取链表
        if (list)
        {                                 // list仍有剩余空间
            freeList[index] = list->next; // 空闲区块指向下一个
            return list;                  // 返回当前区块
        }
        else
        { // 没有足够空间, 需要从内存池中取出空间
            return refill(ROUND_UP(bytes));
        }
    }

    template <bool threads, int inst>
    void alloc<threads, inst>::deAllocate(size_t bytes, void *p)
    {
        if (bytes > MAXBYTES)
        {
            free(p);
            return;
        }

        size_t index = FREELIST_INDEX(bytes);
        obj *node = static_cast<obj *>(p);
        node->next = freeList[index]; // 挂回链表
        freeList[index] = node;
    }

    template <bool threads, int inst>
    void *alloc<threads, inst>::reAllocate(size_t oldSize, size_t newSize, void *p)
    {
        if (oldSize == newSize)
            return p;

        void *result = allocate(newSize);
        size_t copySize = oldSize < newSize ? oldSize : newSize;
        memcpy(result, p, copySize); // 先搬运数据

        deAllocate(oldSize, p); // 再挂回内存

        return result; // 返回新内存空间
    }

    // 将连续内存缝合为单向链接链表
    // 返回一个大小为n的对象，并且有时候会为适当的free list增加节点
    // 假设bytes已经上调为8的倍数
    template <bool threads, int inst>
    void *alloc<threads, inst>::refill(size_t bytes)
    {
        size_t nobjs{alloc<threads, inst>::NOBJS};

        // 从内存池中取
        char *chunk = chunkAlloc(bytes, nobjs); // 注意nobjs传入引用
        obj **myFreeList{nullptr};
        obj *result{nullptr};
        obj *currentObj{nullptr}, *nextObj{nullptr};

        if (nobjs == 1) // 取出的空间只够一个对象使用
            return chunk;

        // 否则调整freeList, 纳入新节点
        myFreeList = freeList + FREELIST_INDEX(bytes);

        result = reinterpret_cast<obj *>(chunk); // 这块还给用户

        // 引导freeList指向新配置的空间(由内存池而来)
        *myFreeList = nextObj = reinterpret_cast<obj *>(chunk + bytes);
        // 串接个节点
        for (size_t i = 1;; i++)
        { // 从1开始, 0返回给用户
            currentObj = nextObj;
            nextObj = reinterpret_cast<obj *>(reinterpret_cast<char *>(nextObj) + bytes);
            if (nobjs - 1 == i)
            {
                currentObj->next = nullptr;
                break;
            }
            else
            {
                currentObj->next = nextObj;
            }
        }

        return result;
    }

    // 切割内存分给refill
    template <bool threads, int inst>
    char *alloc<threads, inst>::chunkAlloc(size_t bytes, size_t &nobjs)
    {
        char *result{nullptr};
        size_t totalBytes{bytes * nobjs};
        ptrdiff_t bytesLeft{endFree - startFree};

        if (static_cast<size_t>(bytesLeft) >= totalBytes)
        {
            // 剩余空间完全满足需求量
            result = startFree;
            startFree += totalBytes;
            return result;
        }
        else if (static_cast<size_t>(bytesLeft) >= bytes)
        {
            // 剩余空间不满足, 但可供应一个以上的区块
            nobjs = bytesLeft / bytes; // 分不出20个了， 改变nojbs
            totalBytes = nobjs * bytes;
            result = startFree;
            startFree += totalBytes;
            return result;
        }
        else
        {
            // 剩余空间连一个区块都无法分割
            size_t bytesToGet{2 * totalBytes + ROUND_UP(heapSize >> 4)};
            // 榨干内存池的剩余零头
            if (bytesLeft > 0)
            {
                obj **myFreeList = freeList + FREELIST_INDEX(bytesLeft);
                // 挂入残留内存
                reinterpret_cast<obj *>(startFree)->next = *myFreeList;
                *myFreeList = reinterpret_cast<obj *>(startFree);
            }

            // 配置heap空间, 补充内存池
            startFree = (char *)malloc(bytesToGet);

            // 系统不给内存了, 前往freeList化缘
            if (!startFree)
            {
                // heap不足, malloc失败
                obj **myFreeList{nullptr}, *p{nullptr};

                for (size_t i = bytes; i <= MAXBYTES; i += ALIGN)
                {
                    myFreeList = freeList + FREELIST_INDEX(i);
                    p = *myFreeList;

                    if (p != nullptr)
                    {
                        // freeList仍有剩余零头未被使用
                        *myFreeList = p->next;                   // freeList头节点移动
                        startFree = reinterpret_cast<char *>(p); // 将零头并入内存池
                        endFree = startFree + i;
                        return chunkAlloc(bytes, nobjs); // 递归调用自己, 看看能否解决内存不足
                    }
                }
                endFree = nullptr;      // 山穷水尽, 到处都没有内存用了
                throw std::bad_alloc(); // 抛出异常, 内存不够了
            }

            // 系统给内存了, 递归调用自己
            heapSize += bytesToGet;
            endFree = startFree + bytesToGet;
            return chunkAlloc(bytes, nobjs); // 递归调用自己, 修正nobjs
        }
    }
};

#endif