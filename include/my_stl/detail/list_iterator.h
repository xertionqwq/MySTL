#ifndef _LIST_ITERATOR_H_
#define _LIST_ITERATOR_H_
#pragma once

#include "my_stl/core/typeTraits.h"
#include "my_stl/core/iterator.h"
#include "my_stl/detail/list_node.h"

namespace MySTL
{
    //---------list迭代器------------
    template <class T, class Ref, class Ptr>
    struct _list_iterator : public MySTL::iterator<MySTL::bidirectional_iterator_tag, T, ptrdiff_t, Ptr, Ref>
    {
        //------------内嵌别名---------------
        using iterator = _list_iterator<T, Ref, Ptr>;
        using link_type = _list_node<T> *; // 指针类型
        link_type node;

        //----------constructor----------
        _list_iterator() = default;
        _list_iterator(link_type val) : node(val) {}
        _list_iterator(const iterator &val) : node(val.node) {}
        template <class OtherRef, class OtherPtr>
        _list_iterator(const _list_iterator<T, OtherRef, OtherPtr> &val) : node(val.node) {}

        //----------operator----------
        bool operator==(const iterator &val) const { return this->node == val.node; }
        bool operator!=(const iterator &val) const { return !(*this == val); }
        typename iterator::reference operator*() const { return (*node).data; }
        typename iterator::pointer operator->() const { return &(this->operator*()); }
        iterator &operator++() {
            node = node->next;
            return *this;
        }
        iterator operator++(int) {
            auto temp = *this;
            ++(*this);
            return temp;
        }
        iterator &operator--() {
            node = node->prev;
            return *this;
        }
        iterator operator--(int) {
            auto temp = *this;
            --(*this);
            return temp;
        }
    };

}; // namespace MySTL

#endif