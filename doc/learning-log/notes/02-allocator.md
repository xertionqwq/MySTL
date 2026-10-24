# 笔记：空间配置器 alloc（内存池 + free-list）

- 编号：02
- 完成日期：2026-10-24
- 对应源码：[alloc.h](../../../include/my_stl/core/alloc.h)
- 实验代码：[experiments/02-topic/experiment.cpp](../experiments/02-topic/experiment.cpp)
- 原目标文档：已删除（内容已整理进本笔记）

## 一句话总结

`alloc` 用两级策略管理字节级内存：≤128 字节的小块交给 16 条 free-list（8 字节步长）复用，free-list 空了就从内存池 `chunkAlloc` 批量补充；>128 字节直接 `malloc/free`。它减少的是小块频繁分配的系统调用和碎片，代价是不支持过对齐类型、不保证线程安全，而且 `deAllocate` 必须传回与 `allocate` 相同的字节数。

## 核心概念

- **分配与构造分离**：`alloc` 只处理"字节 + 对齐"；对象构造/析构由 `construct.h` 和 `allocator<T>` 负责。
- **16 条 free-list**：索引 `i` 管理 `(i+1)*8` 字节的块，覆盖 8~128 字节。空闲块头部用 `union obj` 复用，不额外占内存。
- **free-list 命中**：`allocate` 直接摘链表头，O(1)。
- **free-list 未命中**：`refill(ROUND_UP(bytes))` 从内存池切出一批块，第一块返回用户，其余串成链表。
- **内存池**：`[startFree, endFree)` 是当前这次 `malloc` 得到、尚未切分的区域；`chunkAlloc` 负责从池里切块。
- **增长公式**：`bytesToGet = 2 * totalBytes + ROUND_UP(heapSize >> 4)`。`2 * totalBytes` 保证当前一批 + 下一批；`heapSize >> 4` 是累计申请量的 1/16，作为增长补偿。
- **`heapSize`**：累计向系统申请过的字节数，只是计数器，**不是指针、不等于 `endFree`**，也不保证池地址连续。
- **大块直通**：`> MAXBYTES` 直接 `malloc`/`free`，不经过任何链表。
- **边界**：不支持 `alignas(16/32)` 等过对齐类型；静态状态无锁，非线程安全；小块释放后不归还系统。

## 关键代码路径

- [alloc.h:20-23](../../../include/my_stl/core/alloc.h#L20-L23)：`ALIGN = 8`、`MAXBYTES = 128`、`NFREELISTS = 16`、`NOBJS = 20`
- [alloc.h:27-34](../../../include/my_stl/core/alloc.h#L27-L34)：`union obj` 与 `freeList[16]`
- [alloc.h:38-46](../../../include/my_stl/core/alloc.h#L38-L46)：`FREELIST_INDEX` / `ROUND_UP`
- [alloc.h:91-110](../../../include/my_stl/core/alloc.h#L91-L110)：`allocate`，大块直通 + 链表摘取 + `refill`
- [alloc.h:113-125](../../../include/my_stl/core/alloc.h#L113-L125)：`deAllocate`，小块挂回链表、大块 `free`
- [alloc.h:128-139](../../../include/my_stl/core/alloc.h#L128-L139)：`reAllocate`，分配 + `memcpy` + 释放
- [alloc.h:146-176](../../../include/my_stl/core/alloc.h#L146-L176)：`refill`，第一批块的链表化
- [alloc.h:180-248](../../../include/my_stl/core/alloc.h#L180-L248)：`chunkAlloc` 三分支 + 化缘回退

## 手绘图

### 图 1：refill —— 第一块返回用户，其余串成链表

```
freeList[16]（索引 0~15，每格是 obj*）
  ┌────┬────┬────┬────┬────┬ ... ┬─────┐
  │ 0  │ 1  │ 2  │ 3  │ 4  │     │ 15  │
  └────┴────┴────┴────┴────┴ ... ┴─────┘
            ▲
            │ myFreeList = freeList + FREELIST_INDEX(bytes)
            │
            │ *myFreeList（槽位里存的是块 1 的地址）
            ▼
chunk:  [块0][块1][块2][块3] ... [块 nobjs-1]
          │    ▲
          │    └── cur / nextObj 起点
          └── result，返回客户端，不进入链表

循环 i = 1 .. nobjs-1:
    cur  = next
    next += bytes
    cur->next = next
循环结束:
    cur->next = nullptr
```

- `myFreeList` 是"指向链表头槽位的二级指针"，`*myFreeList` 才是"第一块空闲内存的地址"。
- `nobjs == 1` 时直接返回 `chunk`，不设置链表头、不进入循环。
- 链表里的节点是**原始内存**，只写 `next` 指针，不调用构造函数。

### 图 2：chunkAlloc 的三个分支

记 `totalBytes = bytes * nobjs`，`bytesLeft = endFree - startFree`。

```
① bytesLeft >= totalBytes：池子完全够
   [ startFree ......................... endFree )
   | result（totalBytes）  | 剩余留在池中 |
    ↑                       ↑
    旧 startFree            新 startFree
   res = startFree; startFree += totalBytes;

② bytes >= bytesLeft < totalBytes：够若干块，但不够 nobjs 块
   [ startFree ......................... endFree )
   | result（nobjs*bytes） | 余数（< bytes）|
    ↑                       ↑
    旧 startFree            新 startFree
   nobjs = bytesLeft / bytes;   // 可能变成 3、14……
   totalBytes = nobjs * bytes;
   res = startFree; startFree += totalBytes;
   ** 余数继续留在池中，不挂链表 **

③ bytesLeft < bytes：连一块都不够
   旧池剩余 [startFree, endFree) 整段挂到
       freeList[FREELIST_INDEX(bytesLeft)]
   （bytesLeft 一定是 8 的倍数，所以索引正好对应块大小）

   bytesToGet = 2 * totalBytes + ROUND_UP(heapSize >> 4)
   startFree  = malloc(bytesToGet)
   ├─ 成功：heapSize += bytesToGet; endFree = startFree + bytesToGet;
   │        递归 chunkAlloc —— 这次进入分支 ① 或 ②
   └─ 失败：从 freeList[FREELIST_INDEX(bytes)] 开始，逐条往后找
            p = 链表头; *myFreeList = p->next;
            startFree = p; endFree = p + i;   // i 是该链表原始块大小
            递归 chunkAlloc
            全部为空 → endFree = nullptr; throw std::bad_alloc
```

化缘路径的两个要点：

- **起始索引是 `FREELIST_INDEX(bytes)`，不是 0**。请求 40 字节就从 40 字节链表开始找。
- 拿到的块可能比 `bytes` 大（例如从 64 字节链表拿），所以 `endFree = p + i` 用的是该链表原始块大小 `i`，递归后按实际大小重新切。
- 分支 ③ 刚挂回去的零头 `< bytes`，本次化缘从 `bytes` 开始，因此**不会被本次请求拿走**，留给未来更小的请求。

### 图 3：内存池与 heapSize

```
一次 malloc 得到的 chunk：
[ 已切给用户 / 已挂到 free-list ][ startFree ........ endFree )
                                   \____ bytesLeft ____/

heapSize = 累计所有 bytesToGet 的和
         ≠ endFree 的地址
         ≠ 当前池的结尾
```

### 数值快照：bytes = 8、nobjs = 20

```
totalBytes = 8 * 20 = 160

第 1 次: bytesToGet = 2*160 + ROUND_UP(0)        = 320 → 40 块
第 2 次: bytesToGet = 2*160 + ROUND_UP(320 >> 4) = 344 → 43 块
第 3 次: bytesToGet = 2*160 + ROUND_UP(664 >> 4) = 368 → 46 块
```

累计可提供的 8 字节块数：40、83、129，对应 malloc 次数在第 41、84、130 次分配时增加——与实验五完全吻合。

## 实验与验证

| 实验 | 内容 | 结论 |
|------|------|------|
| test1 | 依次分配 8~128 字节 | 首次 `refill(8)` 拿 20 块；第一块返回，其余入链表 |
| test2 | 反复分配/释放 8 字节 | 地址完全复用，free-list 生效 |
| test3 | 分配 7、14、21… 字节 | 7 字节与 8 字节落入同一链表，验证 `ROUND_UP` |
| test4 | 分配 8、16、129 字节 | 129 直接走 `malloc`，地址与小块区域明显分离 |
| test5 | LD_PRELOAD 统计 malloc | 见下表 |

实验五实测（不含 ASan；总数含 2 次进程/printf 基线）：

| N（8 字节块个数） | malloc 总数 | 增量 | chunk 容量 |
|---|---:|---:|---|
| 0 | 2 | — | 基线 |
| 1~40 | 3 | +1 | 320B = 40 块 |
| 41~83 | 4 | +2 | 344B = 43 块 |
| 84~129 | 5 | +3 | 368B = 46 块 |
| 130 | 6 | +4 | 384B = 48 块 |

补充观察：

- `free` 计数始终为 0：小块 `deAllocate` 只是挂回 free-list，**从不归还系统**。
- 计数时不能加 `-fsanitize=address`，ASan 会拦截 `malloc`，导致计数失真。
- 首次 `malloc` 是 320B 而不是 160B：`2 * totalBytes` 让它一次能服务约 40 个 8 字节块，减少系统调用。

## 踩坑与疑问解答

### 1. 为什么 8 字节对齐？4 字节不行，16 字节不行吗？

64 位下指针、`double`、`int64_t` 的自然对齐都是 8，4 字节会让这些类型错位（未定义行为）。16 字节更安全，但会让最小块粒度翻倍，1~8 字节对象产生大量内部碎片，free-list 数量也要重算。8 是"覆盖常用类型 + 控制碎片"的折中。

注意：`malloc` 返回的 chunk 基址通常 16 对齐，但块步长是 8，**隔一个块就不满足 16 对齐**。因此本配置器不支持 `alignas(16/32)` 的过对齐类型；现代 C++ 用 aligned new / `std::pmr` 处理。

### 2. 最大字节数为什么是 128？256 不行吗？

经验阈值，不是硬规则。超过 128 字节的分配通常次数少、生命周期长，直接 `malloc` 的摊销成本可以接受；如果也放进池里，会长期占住内存、增加最大内部浪费，链表也变多。改成 256 只需把 `NFREELISTS` 调成 32，但碎片和池占用都会上升。现代分配器（jemalloc/tcmalloc）会用更多 size class，本质是同一权衡。

### 3. refill 为什么一次取 20 个？

平衡"系统调用次数"和"只用一个对象时的浪费"。取 1 个等于退化成每次分配都 `malloc`，失去池化意义；取太多则闲置内存多。20 是 SGI 的经验值，配合 `2 * totalBytes` 的申请量，首个 chunk 实际可服务约 40 块。

### 4. 能否用模板参数做成可配置的通用分配器？

可以，把常量提成非类型模板参数：

```cpp
template <bool threads, int inst,
          size_t Align = 8, size_t MaxBytes = 128, size_t Nobjs = 20>
class alloc {
    static constexpr size_t NFREELISTS = MaxBytes / Align;
    static constexpr size_t ALIGN = Align;
    static constexpr size_t MAXBYTES = MaxBytes;
    static constexpr size_t NOBJS = Nobjs;
    // ...
};
```

约束：`Align` 为 2 的幂且不小于目标类型的对齐要求；`MaxBytes` 是 `Align` 的整数倍；`Nobjs > 0`。每个实例化都有独立的静态池（隔离性好，但代码和状态更多）。现代替代方案是 `std::pmr::memory_resource`。

### 5. 为什么 alloc 的函数全是 static？

`alloc` 没有实例状态，所有状态都在静态成员里；调用方（`allocator<T>`）不需要持有对象。实例化一个 `alloc` 对象再调用 `alloc.allocate(...)` 虽然能编过，但对象本身是空的，语义上应写 `alloc::allocate(...)`。线程安全也不是靠实例状态，而是由 `concurrent_allocator` 在外部加静态互斥锁。

### 6. "避免单一定义规则"是什么意思？

非模板类的静态成员如果直接在头文件里定义，被多个翻译单元包含时，链接期会出现重复定义。而**类模板的静态成员**是模板实体，链接器用 COMDAT/弱符号合并，多个 TU 共享同一份定义，所以可以安全地写在头文件里。

`template <bool threads, int inst>` 的两个参数：

- `threads`：选择线程安全版本的开关（当前实现恒为 `false`，线程安全由 `concurrent_allocator` 包装）。
- `inst`：实例编号，允许显式实例化出互不干扰的独立池，也避免不同配置之间的符号冲突。

### 7. 上一版手绘图的三处修正

- **free-list 是 16 条，索引 0~15**，不是 0~16；索引 `i` 对应 `(i+1)*8` 字节。
- **`myFreeList` 与 `*myFreeList` 不是一回事**：前者指向链表头槽位，后者才是第一块空闲内存。
- **分支 ② 有余数**：`nobjs = bytesLeft / bytes` 只切走 `nobjs * bytes`，剩余 `< bytes` 的部分留在池中。

## 自检问题答案

### 1. 为什么 free-list 只管理 8~128 字节？超过 128 为什么不再复用？

小块分配频繁，复用带来的收益（减少 `malloc` 次数、减少碎片）远大于管理成本；大块分配少、生命周期长，池化反而会长期占住内存，且相对浪费更大。128 是经验分界，不是语言限制。

### 2. 为什么 ROUND_UP 要向上取整到 8 的倍数？不取整会怎样？

两个原因：① 保证每个块都能存放需要 8 字节对齐的类型；② 让所有块大小都是 8 的倍数，`FREELIST_INDEX` 才能用简单的整除映射到固定步长的链表。不取整的话，块步长不固定，链表索引和实际块大小会错位，指针也可能不满足对齐要求。

### 3. refill 为什么一次取 20 块而不是 1 块？

取 1 块等于每次分配都触发一次 `malloc`，池化失去意义；一次取 20 块把系统调用摊销到 20 次分配上。20 是经验值，配合 `2 * totalBytes` 的申请量，首批实际可服务约 40 块。

### 4. 内存池剩余空间不足一个块时，零头被放到哪里？

挂到 `freeList[FREELIST_INDEX(bytesLeft)]` 的**链表头部**，作为一整块空闲内存。因为 `bytesLeft` 一定是 8 的倍数，索引正好对应它的块大小；`bytesLeft == 0` 时跳过。这个零头小于当前请求的 `bytes`，本次化缘从 `bytes` 开始找，所以不会被本次请求拿走。

### 5. chunkAlloc 为什么用递归而不是循环？nobjs 引用参数起什么作用？

递归用于"补充池子/借到块之后，重新走一遍完整的三分支判断"，避免把切分逻辑复制一遍；补充后通常直接命中分支 ① 或 ②。

`nobjs` 是引用，因为分支 ② 会把它从 20 改成实际可切的块数（例如 3、14），`refill` 必须知道真实数量，才能正确串接链表；递归调用时这个值也要继续传递。

**纸上模拟**：池里剩 3 个 32 字节块时调用 `refill(32)`：

```
bytes = 32, nobjs = 20, totalBytes = 640, bytesLeft = 96
96 >= 640? 否；96 >= 32? 是 → 分支 ②
nobjs = 96 / 32 = 3
totalBytes = 3 * 32 = 96
result = startFree; startFree += 96  → startFree == endFree
返回 3 个块：第 0 块给用户，块 1、2 串成链表，块 2->next = nullptr
```

池里只剩 24 字节时：

```
24 < 32 → 分支 ③
先把 24 字节零头挂到 freeList[FREELIST_INDEX(24) = 2]（24 字节链表）
再 malloc(bytesToGet)，递归重新切分
```

### 6. 这套 alloc 为什么不是线程安全的？`alloc<false, 0>` 的两个模板参数分别有什么用意？

`freeList`、`startFree`、`endFree`、`heapSize` 都是共享的可变静态状态，没有任何同步；两个线程同时 `allocate` 会竞争同一个链表头或同一段池，导致数据竞争和内存损坏。

- `threads`：意图是选择线程安全实现；当前代码恒用 `false`，真正的线程安全由 `concurrent_allocator` 加互斥锁实现。
- `inst`：实例编号，用于区分不同的独立池/实例化，避免符号冲突，也方便将来做多实例配置。

### 7. allocate 和 deAllocate 的 bytes 参数为什么必须一致？

`deAllocate` 靠 `bytes` 决定两件事：① 挂回哪条 free-list；② 是否 `> 128` 而直接 `free`。分配器没有在每个块前面存 size 头，无法自行恢复真实大小。传错尺寸会把块挂到错误链表（后续分配越界/踩坏数据）或走错释放路径（`free` 一个池内块，或把大块挂进链表）。`allocator<T>` 的 `deallocate(ptr, n)` 用 `sizeof(T) * n` 保证与分配时一致。

## 仍然不确定的地方

- **过对齐类型**：`alignas(16/32)` 的类型如何支持？是否要在 `allocate` 里增加对齐参数，或直接转向 C++17 aligned new / `std::pmr`？
- **长期驻留**：free-list 和池从不把内存还给系统，对长期运行、分配模式多变的程序是否合适？是否需要 shrink 策略？
- **`reAllocate` 的语义**：它用 `memcpy` 搬字节，只适用于平凡可拷贝的原始内存；如果用于有内部资源的对象就是未定义行为。容器里应该用 `uninitialized_copy` + 析构，而不是它。
- **并发化**：`concurrent_allocator` 用一个全局锁串行化所有分配，高并发下会不会成为瓶颈？无锁分配器（如 jemalloc 的 per-thread cache）是如何做的？
- **静态对象的销毁**：`freeList` 和池内存在程序结束时不会回收，ASan/LSan 可能将其视作可达内存；这是池化设计的固有代价还是应该显式清理？
