#ifndef _SKIP_LIST_IMPL_H_
#define _SKIP_LIST_IMPL_H_

#include <random>

namespace MySTL
{
    //======================== 构造 / 析构 ========================

    template <class K, class V>
    skip_list<K, V>::skip_list()
        : head_(createHeadNode()), maxHeight_(1), size_(0)
    {}

    template <class K, class V>
    skip_list<K, V>::~skip_list()
    {
        clear();
        destroyNode(head_);
    }

    //======================== 工厂函数 ========================

    template <class K, class V>
    typename skip_list<K, V>::Node*
    skip_list<K, V>::createNode(const K& k, const V& v, int height)
    {
        size_t bytes = sizeof(Node) + (height - 1) * sizeof(std::atomic<Node*>);

        void* raw = concurrent_allocator::allocate(bytes);
        Node* node = ::new (raw) Node(k, v, height);   // placement new: key, value, next_[0]

        // 初始化额外 forward 指针 (私有阶段, relaxed 即可)
        auto* fwd = &node->next_[0];
        for (int i = 1; i < height; ++i)
            ::new (&fwd[i]) std::atomic<Node*>(nullptr);

        return node;
    }

    template <class K, class V>
    typename skip_list<K, V>::Node*
    skip_list<K, V>::createHeadNode()
    {
        size_t bytes = sizeof(Node) + (MAX_HEIGHT - 1) * sizeof(std::atomic<Node*>);

        void* raw = concurrent_allocator::allocate(bytes);
        Node* node = ::new (raw) Node(MAX_HEIGHT);     // 头节点: 仅设 height, key/value 默认构造

        auto* fwd = &node->next_[0];
        for (int i = 0; i < MAX_HEIGHT; ++i)
            ::new (&fwd[i]) std::atomic<Node*>(nullptr);

        return node;
    }

    template <class K, class V>
    void skip_list<K, V>::destroyNode(Node* node)
    {
        int h = node->height;

        auto* fwd = &node->next_[0];
        for (int i = 0; i < h; ++i)
            fwd[i].~atomic<Node*>();

        node->key.~K();
        node->value.~V();

        size_t bytes = sizeof(Node) + (h - 1) * sizeof(std::atomic<Node*>);
        concurrent_allocator::deAllocate(bytes, node);
    }

    //======================== 随机高度 ========================

    template <class K, class V>
    int skip_list<K, V>::randomHeight()
    {
        thread_local std::mt19937 gen(std::random_device{}());
        thread_local std::uniform_int_distribution<int> dist(0, P_DENOM - 1);

        int h = 1;
        while (h < MAX_HEIGHT && dist(gen) == 0)
            ++h;
        return h;
    }

    //======================== 查找（锁内） ========================

    template <class K, class V>
    bool skip_list<K, V>::findInternal(const K& key, Node** update)
    {
        Node* cur = head_;
        for (int i = maxHeight_ - 1; i >= 0; --i)
        {
            Node* next = cur->next_[i].load(std::memory_order_relaxed);
            while (next && next->key < key)
            {
                cur = next;
                next = cur->next_[i].load(std::memory_order_relaxed);
            }
            update[i] = cur;         // 记录每层前驱
        }
        Node* target = cur->next_[0].load(std::memory_order_relaxed);
        return target && target->key == key;
    }

    //======================== 查找（公开，无锁） ========================

    template <class K, class V>
    V* skip_list<K, V>::find(const K& key)
    {
        Node* cur = head_;
        for (int i = maxHeight_ - 1; i >= 0; --i)
        {
            Node* next = cur->next_[i].load(std::memory_order_acquire);
            while (next && next->key < key)
            {
                cur = next;
                next = cur->next_[i].load(std::memory_order_acquire);
            }
        }
        Node* target = cur->next_[0].load(std::memory_order_acquire);
        return (target && target->key == key) ? &target->value : nullptr;
    }

    template <class K, class V>
    const V* skip_list<K, V>::find(const K& key) const
    {
        Node* cur = head_;
        for (int i = maxHeight_ - 1; i >= 0; --i)
        {
            Node* next = cur->next_[i].load(std::memory_order_acquire);
            while (next && next->key < key)
            {
                cur = next;
                next = cur->next_[i].load(std::memory_order_acquire);
            }
        }
        Node* target = cur->next_[0].load(std::memory_order_acquire);
        return (target && target->key == key) ? &target->value : nullptr;
    }

    //======================== 插入（乐观分配） ========================

    template <class K, class V>
    bool skip_list<K, V>::insert(const K& key, const V& value)
    {
        int h = randomHeight();
        Node* node = createNode(key, value, h);   // 乐观: 锁外分配

        Node* update[MAX_HEIGHT];
        {
            std::lock_guard<std::mutex> lock(mtx_);

            if (findInternal(key, update))         // key 已存在
            {
                destroyNode(node);                 // 回滚预分配
                return false;
            }

            // 提升 maxHeight_
            if (h > maxHeight_)
            {
                for (int i = maxHeight_; i < h; ++i)
                    update[i] = head_;
                maxHeight_ = h;
            }

            // 指针手术: 逐层挂入
            for (int i = 0; i < h; ++i)
            {
                node->next_[i].store(
                    update[i]->next_[i].load(std::memory_order_relaxed),
                    std::memory_order_relaxed
                );
                update[i]->next_[i].store(node, std::memory_order_release);
            }
        }
        size_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    //======================== 删除 ========================

    template <class K, class V>
    bool skip_list<K, V>::erase(const K& key)
    {
        Node* target = nullptr;
        Node* update[MAX_HEIGHT];
        {
            std::lock_guard<std::mutex> lock(mtx_);

            if (!findInternal(key, update))
                return false;

            target = update[0]->next_[0].load(std::memory_order_relaxed);

            // 逐层摘除
            for (int i = 0; i < target->height; ++i)
            {
                update[i]->next_[i].store(
                    target->next_[i].load(std::memory_order_relaxed),
                    std::memory_order_release
                );
            }

            // 收缩 maxHeight_
            while (maxHeight_ > 1 &&
                   head_->next_[maxHeight_ - 1].load(std::memory_order_relaxed) == nullptr)
                --maxHeight_;
        }

        destroyNode(target);
        size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    //======================== 清空 ========================

    template <class K, class V>
    void skip_list<K, V>::clear()
    {
        Node* cur = head_->next_[0].load(std::memory_order_relaxed);
        while (cur)
        {
            Node* next = cur->next_[0].load(std::memory_order_relaxed);
            destroyNode(cur);
            cur = next;
        }

        // 重置 head 的 forward 指针
        for (int i = 0; i < MAX_HEIGHT; ++i)
            head_->next_[i].store(nullptr, std::memory_order_relaxed);

        maxHeight_ = 1;
        size_.store(0, std::memory_order_relaxed);
    }

}

#endif
