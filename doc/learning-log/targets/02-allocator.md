# 目标：空间配置器 alloc（内存池 + free-list）

- 编号：02
- 对应源码：[alloc.h](../../../include/my_stl/core/alloc.h)
- 前置知识：阶段 01（指针、placement new 可选）、类模板与 static 成员的基本语法
- 开始日期：
- 完成标准：能画出一次 `allocate(24)` 从 free-list / 内存池 / `malloc` 的完整路径

## 学习目标

1. 理解 SGI 二级配置器的设计动机：小块复用、大块直通。
2. 能手工模拟 `refill` 和 `chunkAlloc` 的指针变化。
3. 说清楚这套实现的边界：非线程安全、8 字节对齐、128 字节上限。

## 知识点清单

- [ ] 16 条 free-list 与 8 字节粒度：`FREELIST_INDEX` / `ROUND_UP`
- [ ] `union obj` 复用空闲块头部，零额外开销
- [ ] `allocate`：链表头摘取 O(1)；链表空则 `refill`
- [ ] `refill`：一次取 `NOBJS = 20` 块，第一块返回用户，其余串成链表
- [ ] `chunkAlloc` 三种情况：整块够 / 只够若干块 / 一块都不够
- [ ] 内存池剩余零头如何挂回对应 free-list
- [ ] 向系统申请量 `2 * totalBytes + ROUND_UP(heapSize >> 4)`
- [ ] `malloc` 失败时向 free-list “化缘”并递归重试
- [ ] 彻底失败抛 `std::bad_alloc`
- [ ] `deAllocate`：小块挂回链表，大块直接 `free`
- [ ] `reAllocate`：分配 + `memcpy` + 释放
- [ ] `> MAXBYTES` 直接走 `malloc`
- [ ] 静态成员的定义与初始化（类外定义）

## 阅读顺序

1. 先读常量、`union obj`、`freeList`、`FREELIST_INDEX`、`ROUND_UP`
2. 读 `allocate` / `deAllocate` / `reAllocate`
3. 读 `refill`，画出 20 个块如何串起来
4. 读 `chunkAlloc`，分三种分支各走一遍
5. 最后看类外静态成员定义（语法细节，不影响理解）

## 配套 Effective Modern C++

> 仓库：[../../../../Effective_Modern_Cpp](../../../../Effective_Modern_Cpp/README.md)，完整映射见 [reference-map.md](../reference-map.md)

- 基础 13 / 14 / 15：堆栈内存、申请内存的各种方式、重写 operator new/delete
- **基础 16：Array new/delete 与 std::allocator 的引入**（本阶段最直接）
- 条款 14：noexcept（哪些分配路径可以标、哪些不能）
- 条款 15：constexpr（`ALIGN`、`MAXBYTES`、`NOBJS`）

实践方式：看基础 16 时，把 `new[]`、`std::allocator`、MySTL 的
`allocate + construct` 三种方式列成表，说明各自做几件事、适合什么场景。

## 动手实验

- [ ] 写程序依次 `allocate(8)`、`allocate(16)`、…、`allocate(128)`，打印地址，观察是否来自同一段内存
- [ ] 申请若干块后全部释放，再申请同尺寸，验证地址复用
- [ ] 观察 `allocate(129)` 的地址与小块区域的差异
- [ ] 用 `bench/malloc_counter.c`（`LD_PRELOAD`）统计一次批量分配实际调用了多少次 `malloc`
- [ ] 在纸上模拟：当内存池只剩 3 个块时，`refill(32)` 会发生什么

## 自检问题

1. 为什么 free-list 只管理 8~128 字节？超过 128 字节为什么不复用？
2. 为什么 `ROUND_UP` 要向上取整到 8 的倍数？不取整会怎样？
3. `refill` 为什么一次取 20 块而不是 1 块？
4. 内存池剩余空间不足一个块时，零头被放到了哪里？
5. `chunkAlloc` 为什么用递归而不是循环？`nobjs` 引用参数起什么作用？
6. 这套 alloc 为什么不是线程安全的？`alloc<false, 0>` 的两个模板参数分别有什么用意？
7. `allocate` 和 `deAllocate` 的 `bytes` 参数为什么必须一致？

## 学习中的疑问

<!-- 边学边记 -->
