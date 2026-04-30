#ifndef _VECTOR_IMPL_H_
#define _VECTOR_IMPL_H_

#include "typeTraits.h"
#include "allocator.h"
#include "iterator.h"
#include "uninitialized.h"

namespace MySTL
{
    //--------------构造 && 析构函数--------------
    template <class T, class Alloc>
    vector<T, Alloc>::vector(const size_type n)
    {
        allocate_and_fill_n(n, value_type());
    }
    template <class T, class Alloc>
    vector<T, Alloc>::vector(const size_type n, const_reference value)
    {
        allocate_and_fill_n(n, value);
    }
    template <class T, class Alloc>
    vector<T, Alloc>::vector(const _vector &other)
    {
        allocate_and_copy(other.start_, other.finish_);
    }
    template <class T, class Alloc>
    vector<T, Alloc>::vector(_vector &&other) noexcept
        : start_(other.start_), finish_(other.finish_), endOfStorage_(other.endOfStorage_)
    {
        other.start_ = other.finish_ = other.endOfStorage_ = nullptr;
    }
    template <class T, class Alloc>
    vector<T, Alloc>::~vector()
    {
        deallocate();
    }

    //----------------构造析构辅助函数-------------------
    template <class T, class Alloc>
    void vector<T, Alloc>::allocate_and_fill_n(const size_type n, const_reference value)
    {
        start_ = dataAllocator::allocate(n); // 分配内存
        try
        {
            uninitialized_fill_n(start_, n, value); // 调用批量构造函数
            endOfStorage_ = finish_ = start_ + n;
        }
        catch (...)
        { // rollback
            dataAllocator::deallocate(start_, n);
            throw;
        }
    }
    template <class T, class Alloc>
    template <class InputIterator>
    void vector<T, Alloc>::allocate_and_copy(InputIterator first, InputIterator last)
    {
        size_type n = static_cast<size_type>(last - first);
        start_ = dataAllocator::allocate(n); // 分配内存
        try
        {
            finish_ = uninitialized_copy(first, last, start_); // 执行拷贝
            endOfStorage_ = finish_;
        }
        catch (...)
        { // rollback
            dataAllocator::deallocate(start_, n);
            throw;
        }
    }
    template <class T, class Alloc>
    void vector<T, Alloc>::deallocate()
    {
        if (start_ != nullptr)
        {
            dataAllocator::destroy(start_, finish_);                                           // 析构对象
            dataAllocator::deallocate(start_, static_cast<size_type>(endOfStorage_ - start_)); // 回收内存
        }
    }

    //-----------重载操作符-------------
    // copy-and-swap: 强异常保证, 自动处理容量和自赋值
    template <class T, class Alloc>
    auto vector<T, Alloc>::operator=(const _vector &other) -> _vector &
    {
        if (this != &other)
        {
            _vector tmp(other); // 拷贝构造, 抛异常则 *this 不受影响
            this->swap(tmp);    // 交换指针, 绝不抛异常; tmp 析构回收旧资源
        }
        return *this;
    }
    template <class T, class Alloc>
    auto vector<T, Alloc>::operator=(_vector &&other) noexcept -> _vector &
    {
        this->swap(other);
        return *this;
    }
    // operator==: 逐个元素比较
    template <class T, class Alloc>
    bool vector<T, Alloc>::operator==(const _vector &other) const
    {
        if (size() != other.size())
            return false;

        iterator m_begin_ = start_;
        iterator o_begin_ = other.start_;
        for (; m_begin_ != finish_; m_begin_++, o_begin_++)
        {
            if (*m_begin_ != *o_begin_)
                return false;
        }

        return true;
    }
    // operator!=: == 的逻辑逆
    template <class T, class Alloc>
    bool vector<T, Alloc>::operator!=(const _vector &other) const
    {
        return !(*this == other);
    }

    //----------访问元素操作-----------
    template <class T, class Alloc>
    typename vector<T, Alloc>::reference vector<T, Alloc>::at(size_type n)
    {
        if (n >= size())
            throw std::out_of_range("vector::at: n >= size()");
        return *(start_ + n);
    }
    template <class T, class Alloc>
    typename vector<T, Alloc>::const_reference vector<T, Alloc>::at(size_type n) const
    {
        if (n >= size())
            throw std::out_of_range("vector::at: n >= size()");
        return *(start_ + n);
    }

    //----------修改容器函数-----------
    // push_back: 尾部插入单值
    template <class T, class Alloc>
    void vector<T, Alloc>::push_back(const_reference value)
    {
        if (start_ == nullptr)
        {
            // 空 vector，分配初始容量
            start_ = dataAllocator::allocate(1);
            dataAllocator::construct(start_, value);
            finish_ = start_ + 1;
            endOfStorage_ = finish_;
        }
        else if (endOfStorage_ != finish_)
        {
            dataAllocator::construct(finish_, value);
            finish_++;
        }
        else
        {
            insert_aux(finish_, value);
        }
    }
    // pop_back: 尾部弹出
    template <class T, class Alloc>
    void vector<T, Alloc>::pop_back()
    {
        if (!isempty())
        {
            finish_--;
            dataAllocator::destroy(finish_);
        }
    }
    // clear: 清空容器
    template <class T, class Alloc>
    void vector<T, Alloc>::clear()
    {
        if (!isempty())
        {
            dataAllocator::destroy(start_, finish_);
            finish_ = start_;
        }
    }
    // swap: 交换指针
    template <class T, class Alloc>
    void vector<T, Alloc>::swap(_vector &other) noexcept
    {
        iterator tmp = start_;
        start_ = other.start_;
        other.start_ = tmp;
        tmp = finish_;
        finish_ = other.finish_;
        other.finish_ = tmp;
        tmp = endOfStorage_;
        endOfStorage_ = other.endOfStorage_;
        other.endOfStorage_ = tmp;
    }
    // erase: 删除单个元素
    template <class T, class Alloc>
    typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator position)
    {
        return erase(position, position + 1);
    }
    // erase: 删除区间 [first, last)
    template <class T, class Alloc>
    typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator first, iterator last)
    {
        MySTL::copy(last, finish_, first); // 覆盖删除区间
        iterator new_finish_ = finish_ - (last - first);
        dataAllocator::destroy(new_finish_, finish_); // 析构多余元素
        finish_ = new_finish_;                        // 调整迭代器
        return first;
    }
    // insert: 在position前插入单值
    template <class T, class Alloc>
    typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator position, const_reference value)
    {
        difference_type offset = position - start_;
        insert_aux(position, value);
        return start_ + offset;
    }
    // insert: 在position前插入n个value
    template <class T, class Alloc>
    typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator position, size_type n, const_reference value)
    {
        difference_type offset = position - start_;
        insert_aux(position, position + n, value);
        return start_ + offset;
    }
    // insert_aux: 在position前插入单值（内部辅助函数）
    template <class T, class Alloc>
    void vector<T, Alloc>::insert_aux(iterator position, const_reference value)
    {
        if (finish_ != endOfStorage_)
        {
            dataAllocator::construct(finish_, *(finish_ - 1));
            finish_++;
            MySTL::copy_backward(position, finish_ - 2, finish_ - 1);
            *position = value;
        }
        else
        {
            insert_aux(position, position + 1, value);
        }
    }
    // insert_aux: 在区间 [first, last) 前插入n个value（内部辅助函数，含扩容逻辑）
    template <class T, class Alloc>
    void vector<T, Alloc>::insert_aux(iterator first, iterator last, const_reference value)
    {
        const size_type n = static_cast<size_type>(last - first);
        if (n == 0)
            return;
        if (finish_ + n <= endOfStorage_) // 剩余空间足够
        {
            // 先把末尾 n 个元素拷贝到尾部未初始化区
            uninitialized_copy(finish_ - n, finish_, finish_);
            // 然后把 [first, old_finish_) 整体后移到 [first + n, new_finish_)
            if (first != finish_)
            {
                MySTL::copy_backward(first, finish_ - n, finish_);
            }
            // 最后填充空位
            MySTL::fill_n(first, n, value);
            finish_ += n;
        }
        else
        {
            const size_type old_size = size() == 0 ? 1 : size();
            const size_type new_size = old_size * 2 > n ? old_size * 2 : old_size * 2 + n;

            iterator new_start_ = dataAllocator::allocate(new_size);
            iterator new_finish_ = new_start_;
            try
            {
                new_finish_ = uninitialized_copy(start_, first, new_start_); // 移动插入前的元素
                uninitialized_fill_n(new_finish_, n, value);                 // 批量构造插入元素
                new_finish_ += n;                                            // 调整迭代器
                if (first < finish_)
                {
                    new_finish_ = uninitialized_copy(first, finish_, new_finish_);
                }
            }
            catch (...)
            { // rollback
                dataAllocator::destroy(new_start_, new_finish_);
                dataAllocator::deallocate(new_start_, new_size);
                throw;
            }

            deallocate(); // 析构原vector
            // 调整迭代器
            start_ = new_start_;
            finish_ = new_finish_;
            endOfStorage_ = new_start_ + new_size;
        }
    }

    //-----------容量操作------------
    template <class T, class Alloc>
    void vector<T, Alloc>::resize(size_type n, const_reference value)
    {
        if (n < size())
        {
            dataAllocator::destroy(start_ + n, finish_);
            finish_ = start_ + n;
        }
        else if (n > size())
        {
            if (n <= capacity())
            {
                uninitialized_fill_n(finish_, n - size(), value);
                finish_ = start_ + n;
            }
            else
            {
                const size_type new_size = size() * 2 > n ? size() * 2 : n;
                iterator new_start_ = dataAllocator::allocate(new_size);
                iterator new_finish_ = new_start_;
                try
                {
                    new_finish_ = uninitialized_copy(start_, finish_, new_start_);
                    uninitialized_fill_n(new_finish_, n - size(), value);
                    new_finish_ += n - size();
                }
                catch (...)
                {
                    dataAllocator::destroy(new_start_, new_finish_);
                    dataAllocator::deallocate(new_start_, new_size);
                    throw;
                }

                deallocate();
                start_ = new_start_;
                finish_ = new_finish_;
                endOfStorage_ = new_start_ + new_size;
            }
        }
    }
};

#endif