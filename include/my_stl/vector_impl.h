#ifndef _VECTOR_IMPL_H_
#define _VECTOR_IMPL_H_

#include "typeTraits.h"
#include "allocator.h"
#include "iterator.h"
#include "uninitialized.h"

namespace MySTL{
    //--------------构造 && 析构函数--------------
    template<class T, class Alloc>
    void vector<T, Alloc>::allocate_and_fill_n(const size_type n, const_reference value) {
        start_ = dataAllocator::allocate(n); // 分配内存
        _uninitialize_fill_n(start_, n, value); // 调用批量构造函数
        endOfStorage_ = finish_ = start_ + n;
    }
    template<class T, class Alloc>
    vector<T, Alloc>::vector(const size_type n, const T& value) {
        allocate_and_fill_n(n, value);
    }

    template<class T, class Alloc>
    template<class InputIterator>
    void vector<T, Alloc>::allocate_and_copy(InputIterator first, InputIterator last) {
        start_ = dataAllocator::allocate(static_cast<size_type>(last - first)); // 分配内存
        finish_ = uninitalized_copy(first, last, start_); // 执行拷贝
        endOfStorage_ = finish_;
    }
    template<class T, class Alloc>
    vector<T, Alloc>::vector(const _vector &other) {
        allocate_and_copy(other.start_, other.finish_);
    }
};

#endif