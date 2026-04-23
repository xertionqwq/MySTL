#ifndef _VECTOR_H_
#define _VECTOR_H_
#pragma once

#include "allocator.h"
#include "iterator.h"

namespace MySTL
{
    template <class T, class Alloc = allocator<T>>
    class vector
    {
    public:
        //--------------内嵌型别定义-----------------
        using value_type = T;
        using iterator = T *;
        using const_iterator = const T *;
        using reference = T &;
        using const_reference = const T &;
        using pointer = iterator;
        using const_pointer = const_iterator;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using _vector = vector<value_type>;

    private:
        //------------三个迭代器维护空间内存------------
        iterator start_;             // 内存地址的头
        iterator finish_;            // 存放对象的尾
        iterator endOfStorage_;      // 内存地址的尾
        using dataAllocator = Alloc; // 配置空间配置器

    public:
        //-----------构造, 复制, 移动, 析构相关函数----------
        vector() : start_(nullptr), finish_(nullptr), endOfStorage_(nullptr) {}
        explicit vector(const size_type n);
        vector(const size_type n, const_reference value); // 初始化n个value对象
        vector(const _vector &other);                     // 拷贝构造
        vector(_vector &&other);                          // 移动构造
        ~vector();

        //-----------重载操作符-------------
        _vector &operator=(const _vector &other);
        _vector &operator=(_vector &&other);
        bool operator==(const _vector &other) const;
        bool operator!=(const _vector &other) const;

        //-----------迭代器操作-------------
        iterator begin() { return start_; }
        const_iterator begin() const { return start_; }
        iterator end() { return finish_; }
        const_iterator end() const { return finish_; }
        iterator endback() { return endOfStorage_; }
        const_iterator endback() const { return endOfStorage_; }

        //-----------容量操作------------
        size_type size() const { return static_cast<size_type>(end() - begin()); }
        size_type capacity() const { return static_cast<size_type>(endback() - begin()); }
        bool isempty() const { return begin() == end(); };
        void resize(size_type n, const_reference value);

        //-----------访问元素操作-----------
        reference operator[](size_type n) { return *(begin() + n); }
        const_reference operator[](size_type n) const { return *(begin() + n); }
        reference at(size_type n); // 提供边界检查与异常处理
        const_reference at(size_type n) const;
        reference front() { return *begin(); }
        const_reference front() const { return *begin(); }
        reference back() { return *(end() - 1); }
        const_reference back() const { return *(end() - 1); }
        pointer data() { return start_; }
        const_pointer data() const { return start_; }

        //-----------修改容器操作------------
        void push_back(const_reference value);
        void pop_back();
        void clear();
        void swap(_vector &other);
        iterator erase(iterator position);
        iterator insert(iterator position, const_reference value);
        iterator insert(iterator position, size_type n, const_reference value);

        //----------配置器操作---------
        // Alloc get_allocator() { return dataAllocator; }
    private:
        void allocate_and_fill_n(const size_type n, const_reference value);
        template <class InputIterator>
        void allocate_and_copy(InputIterator first, InputIterator last);
        void deallocate();
    private:
        void insert_aux(iterator position, const_reference value);
    };
};

#include "vector_impl.h"

#endif