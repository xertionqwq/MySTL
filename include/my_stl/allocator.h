#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_
#pragma once

#include "alloc.h"
#include "construct.h"

#include <cassert>
#include <new>
#include <limits>

namespace MySTL{
    /*
    ** 空间配置器 allocator
    */
    template<class T>
    class allocator{
    public:
        // 内嵌别名声明
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using difference_type = ptrdiff_t;
        using size_type = size_t;

    public:
        //-------------分开空间分配与对象构造---------------
        // 空间函数声明
        static T *allocate();
        static T *allocate(size_type n);
        static void deallocate(T *ptr);
        static void deallocate(T *ptr, size_type n);

        // 构造函数声明
        static void construct(T *ptr);
        static void construct(T *ptr, const T &value);
        static void destroy(T *ptr);
        static void destroy(T *first, T *last);
    };

    template<class T>
    T *allocator<T>::allocate() {
        return static_cast<T *>(alloc<false, 0>::allocate(sizeof(T)));
    }
    template<class T>
    T *allocator<T>::allocate(size_type n) {
        if (n == 0)
            return nullptr;
        return static_cast<T *>(alloc<false, 0>::allocate(sizeof(T) * n));
    }

    template<class T>
    void allocator<T>::deallocate(T *ptr) {
        alloc<false, 0>::deAllocate(sizeof(T), static_cast<void *>(ptr));
    }
    template<class T>
    void allocator<T>::deallocate(T *ptr, size_type n) {
        alloc<false, 0>::deAllocate(sizeof(T) * n, static_cast<void *>(ptr));
    }

    template<class T>
    void allocator<T>::construct(T *ptr) {
        new (ptr) T(); // 无参, 直接构造
    }
    template<class T>
    void allocator<T>::construct(T *ptr, const T &value) {
        MySTL::construct(ptr, value); // 调用全局构造函数
    }

    template<class T>
    void allocator<T>::destroy(T *ptr) {
        MySTL::destroy(ptr); // 调用全局析构函数
    }
    template<class T>
    void allocator<T>::destroy(T *first, T*last) {
        MySTL::destroy(first, last); // 调用全局析构函数
    }
};


#endif
 