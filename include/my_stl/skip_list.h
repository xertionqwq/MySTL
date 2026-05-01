#ifndef _SKIP_LIST_H_
#define _SKIP_LIST_H_
#pragma once

#include "concurrent_allocator.h"

#include <atomic>
#include <cstddef>
#include <mutex>

namespace MySTL
{
    template <class K, class V>
    class skip_list
    {
    public:
        using size_type = size_t;

    private:
        //----------节点定义----------
        struct Node
        {
            K   key;
            V   value;
            int height;                  // 节点层高 (1..MAX_HEIGHT)
            std::atomic<Node*> next_[1]; // 柔性数组锚点: forward 指针

            Node() = delete;             // 禁止栈上创建 / 默认构造
            Node(const K& k, const V& v, int h)
                : key(k), value(v), height(h), next_{nullptr} {}
            explicit Node(int h)         // 头节点专用: 仅设高度
                : key(), value(), height(h), next_{nullptr} {}
        };

        // 保证跨平台分配计算正确
        static_assert(sizeof(std::atomic<Node*>) == sizeof(Node*),
                      "atomic<Node*> must match Node* size for flexible array");

        static constexpr int   MAX_HEIGHT = 16;
        static constexpr int   P_DENOM    = 4;   // 25% 升级概率

        Node*                head_;       // 哨兵节点, height=MAX_HEIGHT
        int                  maxHeight_;  // 当前实际最高层
        std::atomic<size_type> size_;     // 原子计数器（多线程写）
        std::mutex           mtx_;        // 保护 forward 拓扑结构

    public:
        skip_list();
        ~skip_list();

        // 禁止拷贝（含互斥锁）
        skip_list(const skip_list&) = delete;
        skip_list& operator=(const skip_list&) = delete;

        //-------- 基本操作 --------
        V*        find(const K& key);           // 无锁, acquire 读
        const V*  find(const K& key) const;     // 无锁, const 重载
        bool      insert(const K& key, const V& value);  // 乐观分配 + 锁
        bool      erase(const K& key);          // 锁内摘除
        bool      isempty() const { return size_.load(std::memory_order_relaxed) == 0; }
        size_type size()    const { return size_.load(std::memory_order_relaxed); }

    private:
        //-------- 内部辅助 --------
        int         randomHeight();
        bool        findInternal(const K& key, Node** update); // 锁内, relaxed
        static Node* createNode(const K& k, const V& v, int height);
        static Node* createHeadNode();
        static void destroyNode(Node* node);
        void        clear();
    };

}

#include "skip_list_impl.h"

#endif
