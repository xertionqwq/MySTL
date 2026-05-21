#ifndef _LIST_H_
#define _LIST_H_
#pragma once

#include "my_stl/detail/list_iterator.h"
#include "my_stl/core/algorithm.h"
#include "my_stl/core/allocator.h"
#include "my_stl/core/construct.h"
#include "my_stl/core/uninitialized.h"

namespace MySTL
{
    template <class T, class Alloc = allocator<T>>
    class list
    {
    public:
        //----------内嵌型别定义---------
        using value_type = T;
        using reference = T &;
        using link_type = _list_node<T> *;
        using iterator = _list_iterator<T, T &, T *>;
        using const_iterator = _list_iterator<T, const T &, const T *>;
        using _list = list<value_type>;
        using size_type = size_t;

    protected:
        using list_node = _list_node<T>;
        using dataAllocator = Alloc;
        // 专属空间配置器
        using nodeAllocator = typename dataAllocator::template rebind<list_node>::other;

    protected:
        link_type node; // 哨兵节点, 负责双向循环列表

    protected:
        //----------空间配置器函数----------
        // 配置一个节点空间返回
        link_type get_node()
        {
            return nodeAllocator::allocate();
        }
        // 释放一个节点空间
        void free_node(link_type p)
        {
            nodeAllocator::deallocate(p);
        }
        // 配置并构造一个节点
        link_type create_node(const value_type &val)
        {
            link_type p = get_node();
            nodeAllocator::construct(&p->data, val);
            return p;
        }
        // 析构并回收一个节点
        void destroy_node(link_type p)
        {
            nodeAllocator::destroy(&p->data);
            free_node(p);
        }

    protected:
        //---------辅助函数-----------
        void empty_initialize()
        {
            node = create_node();
            node->prev = nullptr;
            node->next = nullptr;
        }

    public:
        //---------construct&destroy------------
        list() { empty_initialize(); }
        ~list();

        //---------重载符操作---------

        //---------迭代器操作----------
        iterator begin() { return static_cast<iterator>(node->next); }
        const_iterator begin() const { return static_cast<iterator>(node->next); }
        iterator end() { return static_cast<iterator>(node); }
        const_iterator end() const { return static_cast<iterator>(node); }

        //---------容量操作----------
        bool isempty() const { return node == node->next; }
        size_type size() const;

        //---------访问元素操作----------
        reference front() { return *begin(); }
        reference back() { return *(--end()); }
    };
}; // namespace MySTL

#endif