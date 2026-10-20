# Effective Modern C++ × MySTL 对照与实战指南

配套仓库：`../Effective_Modern_Cpp`（下称 EMCPP，使用 C++14 + `-fno-elide-constructors`）

两个仓库的分工：

| 仓库 | 定位 | 用途 |
|------|------|------|
| Effective_Modern_Cpp | 语言规则 + 最小示例 | 搞懂“现代 C++ 为什么这样写” |
| MySTL | 真实工程实现 | 验证“规则在真实代码里怎么落地” |

推荐循环：**EMCPP 学规则 → MySTL 找落点 → 写最小实验验证 → 回填学习笔记**。

## 一、阶段映射

### 阶段 01：C++ 基础与对象生命周期

| EMCPP 内容 | MySTL 落点 | 实战动作 |
|------------|------------|----------|
| 基础 2：值类型与右值引用 | `allocator::construct(T*, T&&)` | 写一个只可移动类型，观察哪个 `construct` 重载被调用 |
| 基础 10：构造函数语义学 | `vector` 的拷贝/移动构造 | 用 `= default` / `= delete` 实验验证特殊成员生成规则 |
| 基础 11：异常处理与 swap & copy | `vector::operator=` 的 copy-and-swap | 让拷贝构造抛异常，验证原对象不变 |
| 基础 13：堆栈内存分配与泄漏 | `alloc::allocate` / `deallocate` | 用 ASan 对比“忘记析构”和“忘记释放”的报错差异 |
| 基础 14：C++ 申请内存的各种方法 | `malloc` + placement new | 手写四种分配/构造组合，打印地址与析构次数 |
| 基础 15：重写 operator new/delete | 为什么 MySTL 不直接重载全局 new | 阅读后说明自定义配置器的优势 |
| 基础 17：RVO / NRVO | 移动构造、按值返回 | 关闭/开启省略，观察移动构造调用次数 |
| 条款 1、7、8、11、17、23、24 | 见下方“重点条款落点” | — |

### 阶段 02：空间配置器 alloc

| EMCPP 内容 | MySTL 落点 | 实战动作 |
|------------|------------|----------|
| 基础 13 / 14 / 15 | [alloc.h](../../include/my_stl/core/alloc.h) | 统计一次批量分配实际触发的 `malloc` 次数 |
| 基础 16：Array new/delete 与 `std::allocator` 的引入 | `allocate` 与 `construct` 分离 | 解释为什么 `new[]` 不能替代 `allocate` + `uninitialized_*` |
| 条款 15：constexpr | `ALIGN`、`MAXBYTES`、`NOBJS` | 给常量加 `static_assert` 验证 |
| 条款 14：noexcept | `deAllocate` / `bad_alloc` 路径 | 思考哪些函数可以标 `noexcept`，哪些不能 |

### 阶段 03：模板萃取、迭代器与批量构造

| EMCPP 内容 | MySTL 落点 | 实战动作 |
|------------|------------|----------|
| 条款 1：理解模板类型推导 | `_type_traits<T>`、`allocator<T>` | 推导 `T&&`、`const T&` 在模板中的实际类型 |
| 条款 2 / 3：auto 与 decltype | `using` 别名、迭代器萃取 | 用 `decltype` 复现 `iterator_traits` 的成员类型 |
| 条款 9：优先使用别名声明 | 全项目的 `using` | 对比 `typedef` 写法 |
| 条款 13：优先使用 const_iterator | `vector::begin() const`、list 迭代器 | 补一个只读遍历实验 |
| 基础 3：数组、函数与指针 | 裸指针作为迭代器 | 验证 `T*` 满足随机访问迭代器 |
| 基础 9：CRTP / Expression Template | traits 的编译期分派思想 | 只理解“编译期选择实现”，不必照搬 |

### 阶段 04：vector 完整实现

| EMCPP 内容 | MySTL 落点 | 实战动作 |
|------------|------------|----------|
| 基础 2 / 条款 23 / 24 | `push_back(const T&)` vs `push_back(T&&)` | 用左值、右值、`const` 右值分别调用，记录重载选择 |
| 基础 11 | copy-and-swap 赋值 | 构造抛异常的类型，验证强异常安全 |
| 基础 17 / 条款 17 | 移动构造与特殊成员函数 | `static_assert(std::is_nothrow_move_constructible_v<...>)` |
| 条款 14：noexcept | `vector(vector&&) noexcept` | 删掉 `noexcept`，观察容器扩容时是否退化为拷贝 |
| 条款 13：const_iterator | const 版本 `begin/end/at` | 检查是否所有只读接口都有 const 重载 |
| 条款 5 / 6 / 7 | `size_type`、初始化 | 用 `auto` 重写测试代码，观察类型推导差异 |

### 后续阶段

- **list**：条款 13、17，基础 10、12（友元与节点访问）。
- **skip_list**：条款 14、17、23、24 可复用；**并发部分 EMCPP 不覆盖**，需另补 `std::atomic`、内存序、`std::thread` 资料。

## 二、重点条款 → 代码落点速查

| 条款 / 基础 | MySTL 中的对应物 |
|-------------|------------------|
| 条款 1 模板类型推导 | `_type_traits` 特化、`allocator::construct` 重载 |
| 条款 11 使用 `= delete` | `skip_list` 禁止拷贝 |
| 条款 13 const_iterator | `vector` 的 const 重载、`_list_iterator<T, const T&, const T*>` |
| 条款 14 noexcept | `vector` 移动构造、`swap` |
| 条款 15 constexpr | `alloc` 的 `ALIGN` / `MAXBYTES`、`skip_list::MAX_HEIGHT` |
| 条款 17 特殊成员函数生成 | `vector` Rule of 3/5、`list` 的析构 |
| 条款 23 move / forward | `push_back(T&&)`、`insert_aux(position, T&&)` |
| 条款 24 通用引用与右值引用 | `push_back` 重载、`construct` 的 `T&&` 重载 |
| 基础 11 swap & copy | `operator=(const vector&)` |
| 基础 16 array new/delete | `allocate` + `uninitialized_*` 代替 `new[]` |

## 三、实验代码放哪里

不要往 MySTL 主库里塞验证代码。建议：

```
doc/learning-log/experiments/NN-topic/
└── experiment.cpp
```

单独编译，避免污染库和主 CMake：

```bash
clang++ -std=c++17 -Wall -Wextra -g -fsanitize=address \
    -I include doc/learning-log/experiments/NN-topic/experiment.cpp \
    -o /tmp/mystl_exp && /tmp/mystl_exp
```

## 四、注意事项

1. **标准不同**：EMCPP 演示仓库用 C++14，且故意 `-fno-elide-constructors`；MySTL 是 C++17。C++17 下部分拷贝省略是强制的，示例现象不能直接套用。
2. **EMCPP 不是全量**：仓库主要覆盖《Effective Modern C++》第 1~2 章及部分条款，缺少模板偏特化、迭代器、并发等内容。阶段 03 的 traits 和阶段 06 的并发需另找资料（《STL 源码剖析》、cppreference）。
3. **不要为了用条款而改代码**：先判断 MySTL 现有实现是否已经符合；不符合时，先记进笔记，再决定是否修改。
4. **每次只做一个小实验**：一个条款对应一个可运行示例，结论写进 `notes/`，比通读一遍更有效。
