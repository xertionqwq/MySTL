#ifndef _UNINITIALIZED_H_
#define _UNINITIALIZED_H_
#pragma once

#include "construct.h"
#include "algorithm.h"
#include "typeTraits.h"
#include "iterator.h"

namespace MySTL{
    /*
    ******uninitialized_copy (大批量拷贝构造)******
    */
    // 底层函数声明, 以便顶层接口函数使用
    template <class InputIterator, class ForwardIterator>
    ForwardIterator _uninitalized_copy_aux(InputIterator first, InputIterator last,
                                           ForwardIterator result, _true_type);
    template <class InputIterator, class ForwardIterator>
    ForwardIterator _uninitalized_copy_aux(InputIterator first, InputIterator last,
                                           ForwardIterator result, _false_type);

    // 顶层接口函数
    template<class InputIterator, class ForwardIterator>
    ForwardIterator uninitalized_copy(InputIterator first, InputIterator last, ForwardIterator result) {
        using value_type = typename iterator_traits<ForwardIterator>::value_type; // 萃取迭代器所指类型
        using isPODType = typename _type_traits<value_type>::is_POD_type; // 萃取型别特征
        return _uninitalized_copy_aux(first, last, result, isPODType{});
    }
    // 底层标签函数, 接收_true_type类型, 调用算法copy
    template<class InputIterator, class ForwardIterator>
    ForwardIterator _uninitalized_copy_aux(InputIterator first, InputIterator last,
    ForwardIterator result, _true_type) {
        return copy(first, last, result);
    }
    // 接收_false_type类型, 逐个调用构造函数,
    template<class InputIterator, class ForwardIterator>
    ForwardIterator _uninitalized_copy_aux(InputIterator first, InputIterator last,
    ForwardIterator result, _false_type) {
        ForwardIterator cur{result};
        try {
            for (; first != last; first++, cur++) {
                construct(&*cur, *first);
            }
            return cur;
        }
        catch(...) {
            // rollback
            destroy(result, cur);
            throw; // 交给顶层函数处理
        }
    }

    /*
    ******uninitialized_fill (大批量填充构造)******
    */
    // 底层函数声明, 以便顶层接口函数使用
    template <class ForwardIterator, class T>
    void _unintialized_fill_aux(ForwardIterator first, ForwardIterator last,
                                const T &value, _true_type);
    template <class ForwardIterator, class T>
    void _unintialized_fill_aux(ForwardIterator first, ForwardIterator last,
                                const T &value, _false_type);
    // 顶层接口函数
    template <class ForwardIterator, class T>
    void _uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& value) {
        using isPODType = typename _type_traits<T>::is_POD_type; // 萃取型别特征
        _unintialized_fill_aux(first, last, value, isPODType{});
    }
    // 底层函数, 接收_true_type类型, 调用算法fill()
    template <class ForwardIterator, class T>
    void _unintialized_fill_aux(ForwardIterator first, ForwardIterator last,
    const T &value, _true_type) {
        fill(first, last, value);
    }
    // 接收_false_type类型, 逐个调用构造函数
    template <class ForwardIterator, class T>
    void _unintialized_fill_aux(ForwardIterator first, ForwardIterator last,
    const T &value, _false_type) {
        ForwardIterator start(first);
        try {
            for (; first != last; first++) {
                construct(&*first, value);
            }
        }
        catch(...) {
            destroy(start, first); // rollback
            throw;
        }
    }

    /*
    ******uninitialized_fill_n (填充指定数量)******
    */
    // 底层函数声明, 以便顶层接口函数使用
    template <class ForwardIterator, class Size, class T>
    ForwardIterator _uninitialized_fill_n_aux(ForwardIterator first, Size n,
                                              const T &value, _true_type);
    template <class ForwardIterator, class Size, class T>
    ForwardIterator _uninitialized_fill_n_aux(ForwardIterator first, Size n,
                                              const T &value, _false_type);
    // 顶层接口函数
    template<class ForwardIterator, class Size, class T>
    ForwardIterator _uninitialize_fill_n(ForwardIterator first, Size n, const T &value) {
        using isPODType = typename _type_traits<T>::is_POD_type;
        return _uninitialized_fill_n_aux(first, n, value, isPODType{});
    }
    // 底层函数, 接收_true_type类型, 调用算法fill_n
    template<class ForwardIterator, class Size, class T>
    ForwardIterator _uninitialized_fill_n_aux(ForwardIterator first, Size n,
    const T &value, _true_type) {
        return fill_n(first, n, value);
    }
    // 接收_false_type类型, 对n个对象逐个调用构造函数
    template<class ForwardIterator, class Size, class T>
    ForwardIterator _uninitialized_fill_n_aux(ForwardIterator first, Size n,
    const T &value, _false_type) {
        ForwardIterator start{first};
        try {
            for (int i = 0; i < n; i++, first++) {
                construct(&*first, value);
            }
            return first;
        }
        catch(...) {
            destroy(start, first); // roolback
            throw;
        }
    }
};

#endif