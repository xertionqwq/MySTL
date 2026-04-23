#ifndef _VECTOR_IMPL_H_
#define _VECTOR_IMPL_H_

#include "typeTraits.h"
#include "allocator.h"
#include "iterator.h"
#include "uninitialized.h"

namespace MySTL{
    //--------------构造 && 析构函数--------------
    template<class T, class Alloc>
    vector<T, Alloc>::vector(const size_type n) {
        allocate_and_fill_n(n, value_type());
    }
    template<class T, class Alloc>
    vector<T, Alloc>::vector(const size_type n, const_reference value) {
        allocate_and_fill_n(n, value);
    }
    template<class T, class Alloc>
    vector<T, Alloc>::vector(const _vector &other) {
        allocate_and_copy(other.start_, other.finish_);
    }
    template<class T, class Alloc>
    vector<T, Alloc>::~vector() {
        deallocate();
    }

    //----------------构造析构辅助函数-------------------
    template<class T, class Alloc>
    void vector<T, Alloc>::allocate_and_fill_n(const size_type n, const_reference value) {
        start_ = dataAllocator::allocate(n); // 分配内存
        try {
            uninitialize_fill_n(start_, n, value); // 调用批量构造函数
            endOfStorage_ = finish_ = start_ + n;
        }
        catch(...) { // rollback
            dataAllocator::deallocate(start_, n);
            throw;
        }
    }
    template<class T, class Alloc>
    template<class InputIterator>
    void vector<T, Alloc>::allocate_and_copy(InputIterator first, InputIterator last) {
        size_type n = static_cast<size_type>(last - first);
        start_ = dataAllocator::allocate(n); // 分配内存
        try {
            finish_ = uninitalized_copy(first, last, start_); // 执行拷贝
            endOfStorage_ = finish_;
        }
        catch(...) { // rollback
            dataAllocator::deallocate(start_, n);
            throw;
        }
    }
    template<class T, class Alloc>
    void vector<T, Alloc>::deallocate() {
        if (start_ != nullptr) {
            dataAllocator::destroy(start_, finish_); // 析构对象
            dataAllocator::deallocate(start_, static_cast<size_type>(endOfStorage_ - start_)); // 回收内存
        }
    }

    //----------修改容器函数-----------
    template <class T, class Alloc>
    void vector<T, Alloc>::push_back(const_reference value) {
        if (endOfStorage_ != finish_) {
            dataAllocator::construct(finish_, value);
            finish_++;
        } else {
            insert_aux(finish_, value);
        }
    }
    template <class T, class Alloc>
    void vector<T, Alloc>::pop_back() {
        if (!isempty()) {
            finish_--;
            dataAllocator::destroy(finish_);
        }
    }
    template <class T, class Alloc>
    void vector<T, Alloc>::clear() {
        if (!isempty()) {
            dataAllocator::destroy(start_, finish_);
            finish_ = start_;
        }
    }
template<class T, class Alloc>
void vector<T, Alloc>::insert_aux(iterator position, const_reference value) {
    if (finish_ != endOfStorage_) {
        dataAllocator::construct(finish_, *(finish_ - 1));
        finish_++; 
        MySTL::copy_backward(position, finish_ - 2, finish_ - 1);
        *position = value; // 插入元素
    } else {
        const size_type old_size = size();
        const size_type new_size = old_size == 0 ? 1 : 2 * old_size; // 原理同标准库，0 扩容为 1

        iterator new_start_ = dataAllocator::allocate(new_size);
        iterator new_finish_ = new_start_;
        try {
            new_finish_ = uninitalized_copy(start_, position, new_start_); // 1. 前半段
            dataAllocator::construct(new_finish_, value);                  // 2. 插入点
            new_finish_++;
            new_finish_ = uninitalized_copy(position, finish_, new_finish_); // 3. 后半段
        }
        catch(...) { 
            dataAllocator::destroy(new_start_, new_finish_); 
            dataAllocator::deallocate(new_start_, new_size); 
            throw;
        }

        // 释放原 vector
        deallocate();

        // 调整迭代器
        start_ = new_start_;
        finish_ = new_finish_;
        endOfStorage_ = start_ + new_size;
    }
}
};

#endif