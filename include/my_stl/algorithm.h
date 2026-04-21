#ifndef _ALGORITHM_H
#define _ALGORITHM_H
#pragma once

#include <cstring>
#include <utility>

#include "iterator.h"

namespace MySTL
{

    /*
    ** fill: 将 [first, last) 区间内的元素全部赋予 value
    */
    template <class ForwardIterator, class T>
    void fill(ForwardIterator first, ForwardIterator last, const T& value) {
        for (; first != last; ++first) {
            *first = value; // POD 类型直接赋值
        }
    }
    // char & wchar_t 偏特化版本
    inline void fill(char *first, char *last, const char &value) {
        memset(first, static_cast<unsigned char>(value), last - first);
    }
    inline void fill(wchar_t *first, wchar_t *last, const wchar_t &value) {
        memset(first, static_cast<unsigned char>(value), (last - first) * sizeof(wchar_t));
    }

    /*
    ** fill_n: 从 first 开始，填充 n 个元素，并返回填充后的尾部迭代器
    */
    template <class OutputIterator, class Size, class T>
    OutputIterator fill_n(OutputIterator first, Size n, const T& value) {
        for (; n > 0; --n, ++first) {
            *first = value;
        }
        return first; // 注意：返回的是填充完毕后的下一个位置
    }

    /*
    ** copy: 将 [first, last) 的内容拷贝到以 result 为起点的空间
    ** 返回值: 目标空间的尾部迭代器 (即 result + (last - first))
    */
    template <class InputIterator, class OutputIterator>
    OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result) {
        for (; first != last; ++first, ++result) {
            *result = *first;
        }
        return result; 
    }
}

#endif