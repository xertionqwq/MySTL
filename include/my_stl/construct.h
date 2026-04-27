#ifndef _CONSTRUCT_H_
#define _CONSTRUCT_H_
#pragma once

#include <new>
#include "typeTraits.h"
#include "iterator.h"

/*
** 负责对象的构造和析构
*/

namespace MySTL
{

    template <class T1, class T2>
    inline void construct(T1 *ptr, const T2 &value)
    {
        new (ptr) T1(value);
    }

    template <class T> // 析构第一版, 单个直接析构
    inline void destroy(T *ptr)
    {
        ptr->~T();
    }

    template <class ForwardIterator> // 第二版, 判断数值型别
    inline void destroy(ForwardIterator first, ForwardIterator last)
    {
        // 萃取迭代器所指对象
        using ValueType = typename iterator_traits<ForwardIterator>::value_type;
        using is_POD_type = typename _type_traits<ValueType>::is_POD_type;
        _destroy(first, last, is_POD_type{});
    }

    template <class ForwardIterator> // value type并非POD——type, 逐个析构
    inline void _destroy(ForwardIterator first, ForwardIterator last, _false_type)
    {
        for (; first != last; first++)
        {
            destroy(&*first);
        }
    }

    template <class ForwardIterator> // 是POD_type, 什么都不做
    inline void _destroy(ForwardIterator first, ForwardIterator last, _true_type)
    {
        (void)first;
        (void)last;
    }
};

#endif
