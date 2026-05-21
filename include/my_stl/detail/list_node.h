#ifndef _LIST_NODE_H_
#define _LIST_NODE_H_
#pragma once

namespace MySTL{
//----------双向链表节点-------------
template <class T>
struct _list_node {
    using link_type = _list_node<T> *;
    link_type prev;   // 前驱指针
    link_type next;   // 后驱指针
    T data;

};
}; // namespace MySTL

#endif