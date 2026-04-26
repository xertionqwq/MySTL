#ifndef _ITERATOR_H_
#define _ITERATOR_H_
#pragma once

namespace MySTL
{

    // 五种迭代器类型, 继承以使用自动降级和标签分发
    struct input_iterator_tag
    {
    };
    struct output_iterator_tag
    {
    };
    struct forward_iterator_tag : public input_iterator_tag
    {
    };
    struct bidirectional_iterator_tag : public forward_iterator_tag
    {
    };
    struct random_access_iterator_tag : public bidirectional_iterator_tag
    {
    };

    // 迭代器身份证明
    template <class Category, class T, class Distance = ptrdiff_t,
              class Pointer = T *, class Reference = T &>
    struct iterator
    {
        using iterator_category = Category;
        using value_type = T;
        using difference_type = Distance;
        using pointer = T *;
        using reference = T &;
    };

    template <class T, class Distance>
    struct input_iterator
    {
        using iterator_category = input_iterator_tag;
        using value_type = T;
        using difference_type = Distance;
        using pointer = T *;
        using reference = T &;
    };

    struct output_iterator
    {
        using iterator_category = output_iterator_tag;
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
    };

    template <class T, class Distance>
    struct forward_iterator
    {
        using iterator_category = forward_iterator_tag;
        using value_type = T;
        using difference_type = Distance;
        using pointer = T *;
        using reference = T &;
    };

    template <class T, class Distance>
    struct bidirectional_iterator
    {
        using iterator_category = bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = Distance;
        using pointer = T *;
        using reference = T &;
    };

    template <class T, class Distance>
    struct random_access_iterator
    {
        using iterator_category = random_access_iterator_tag;
        using value_type = T;
        using difference_type = Distance;
        using pointer = T *;
        using reference = T &;
    };

    /*
    ** 迭代器萃取机
    */
    template <class Iterator>
    struct iterator_traits
    {
        using iterator_category = typename Iterator::iterator_category;
        using value_type = typename Iterator::value_type;
        using difference_type = typename Iterator::difference_type;
        using pointer = typename Iterator::pointer;
        using reference = typename Iterator::reference;
    };

    // 原生指针偏特化版
    template <class T>
    struct iterator_traits<T *>
    {
        using iterator_category = random_access_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using reference = T &;
    };

    // 原生底层指针偏特化版
    template <class T>
    struct iterator_traits<const T *>
    {
        using iterator_category = random_access_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;
    };

    // 决定迭代器类型 category
    template <class Iterator>
    inline typename iterator_traits<Iterator>::iterator_category
    iterator_category(const Iterator &It)
    {
        using category = typename iterator_traits<Iterator>::iterator_category;
        return category();
    }
    // 与上同理
    template <class Iterator>
    inline typename iterator_traits<Iterator>::iterator_category
    value_type(const Iterator &It)
    {
        return static_cast<typename iterator_traits<Iterator>::value_type *>(nullptr);
    }
    // 与上同理
    template <class Iterator>
    inline typename iterator_traits<Iterator>::difference_type
    difference_type(const Iterator &It)
    {
        return static_cast<typename iterator_traits<Iterator>::difference_type *>(nullptr);
    }
};

#endif