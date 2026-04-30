# My STL

C++ STL 源码级复刻，参考侯捷《STL 源码剖析》。C++17，仅头文件，CMake 构建。

```cpp
#include "my_stl/vector.h"
MySTL::vector<int> v{1, 2, 3};
v.push_back(4);
```

## 快速开始

```bash
cd build && cmake .. && make -j$(nproc)   # 编译
./tests/test_vector                        # 运行测试
# 或一键: ./scripts/run_tests_vector.sh   # 输出到 output/run_vector_log.md
```

编译选项：`-Wall -Wextra -g -fsanitize=address`。

## 项目结构

```
include/my_stl/
├── alloc.h              # 二级空间配置器（内存池 + free-list）
├── allocator.h          # 类型安全封装，分离分配与构造
├── construct.h          # 对象构造/析构，基于 type_traits 分派
├── typeTraits.h         # 型别特征萃取（POD 判断）
├── iterator.h           # 迭代器萃取，五种标签
├── uninitialized.h      # 批量初始化（copy / fill / fill_n）
├── algorithm.h          # 基础算法（copy / copy_backward / fill / fill_n）
├── vector.h             # vector 声明
└── vector_impl.h        # vector 实现
tests/
├── test_vector.cpp      # 55 用例
└── test_allocator.cpp   # 空壳，待编写
scripts/
└── run_tests_vector.sh
```

## 实现进度

| 模块 | 状态 |
|------|------|
| 空间配置器（alloc / allocator） | ✅ |
| 类型萃取（typeTraits） | ✅ |
| 迭代器萃取（iterator） | ✅ |
| 构造/析构工具（construct） | ✅ |
| 批量初始化（uninitialized） | ✅ |
| 基础算法（algorithm） | ✅ |
| vector — 构造/析构 | ✅ |
| vector — push_back / pop_back / clear | ✅ |
| vector — resize（标准语义） | ✅ |
| vector — erase（单元素 + 区间） | ✅ |
| vector — insert（单值 + n 值） | ✅ |
| vector — 拷贝/移动构造与赋值 | ✅ |
| vector — swap | ✅ |
| vector — operator== / != | ✅ |
| vector — at（边界检查） | ✅ |
| list | ⬜ |
| deque | ⬜ |

## 设计决策

### 空间配置器：为什么分两层

`alloc` 处理字节级内存，`allocator<T>` 包装为类型安全接口。

- **free-list**：16 条链表管理 8~128 字节小块，同尺寸复用避免碎片和系统调用
- **内存池**：一次向系统 `malloc` 20 个同尺寸块，后续分配只摘链表头（O(1)）
- **>128 字节**：直接走 `malloc`/`free`，不经过内存池

对比：若每个对象单独 `malloc`，频繁插入删除会产生大量系统调用；若全部走内存池，大对象会浪费池空间。两级方案各取所长。

### vector 拷贝赋值：为什么用 copy-and-swap

```cpp
operator=(const vector& other) {
    if (this != &other) {
        vector tmp(other);   // 拷贝构造，抛异常则 *this 不变
        this->swap(tmp);     // 交换指针，绝不抛异常
    }                         // tmp 析构回收旧资源
    return *this;
}
```

| 方案 | 异常安全 | 容量处理 | 自赋值 |
|------|---------|---------|--------|
| 原地 destroy + copy | 弱：destroy 后 copy 抛异常则状态损坏 | 需手动检查并扩容 | 需额外判断 |
| copy-and-swap | 强：临时对象构造失败则 *this 不变 | 自动处理 | 指针比较即可 |

### vector 移动构造：为什么直接操作指针而非 swap

```cpp
vector(vector&& other) noexcept
    : start_(other.start_), finish_(other.finish_), endOfStorage_(other.endOfStorage_)
{
    other.start_ = other.finish_ = other.endOfStorage_ = nullptr;
}
```

移动构造语义是"盗取即将死亡对象"，不需要交换。swap 是独立的公共接口，用于两个活跃对象互相交换内容。职责分离，避免间接层。

### operator!=：为什么归一到 ==

```cpp
bool operator!=(const vector& other) const {
    return !(*this == other);   // 单一真相源，避免两套比较逻辑不同步
}
```

## 命名约定

遵循 SGI STL 风格：

- **成员变量**：下划线后缀（`start_`、`finish_`、`endOfStorage_`）
- **类型萃取**：`_type_traits`、`_true_type` / `_false_type`
- **迭代器萃取**：`iterator_traits`，五种标签 `input_iterator_tag` / `output_iterator_tag` / ...
- **分配与构造**：走 `dataAllocator` 包装，`allocate` 拿空间，`construct` 做构造
- **算法函数**：`MySTL::copy`、`MySTL::fill_n` 等
- **批量初始化**：`uninitialized_copy`、`uninitialized_fill_n`，不走 `MySTL::` 前缀

## 已知限制

- **单线程**：`alloc<false, 0>`，无锁实现
- **128 字节上限**：超过的分配直接走 `malloc`，不复用 free-list
- **8 字节对齐**：所有小块分配向上取整到 8 的倍数，有内部碎片
- **无重入保护**：`chunkAlloc` 中 `malloc` 失败后从 free-list 拿块回填内存池的逻辑只在单线程可用
- **未实现迭代器抽象**：`vector::iterator` 目前是裸指针 `T*`

## 参考资源

- 侯捷《STL 源码剖析》
- [SGI STL](https://github.com/steveLauwh/SGI-STL)
- [LLVM libc++](https://github.com/llvm/llvm-project/tree/main/libcxx)
