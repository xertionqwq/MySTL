# 目标：vector 完整实现

- 编号：04
- 对应源码：[vector.h](../../../include/my_stl/containers/vector.h)、[vector_impl.h](../../../include/my_stl/detail/vector_impl.h)
- 测试规格：[test_vector.cpp](../../../tests/test_vector.cpp)（54 个用例）
- 前置知识：阶段 01–03
- 开始日期：
- 完成标准：能白板写出三指针模型和扩容流程，并解释每个异常路径为什么安全

## 学习目标

1. 掌握三指针模型下的 size / capacity / 迭代器运算。
2. 理解 2 倍扩容、元素搬移和异常回滚的完整过程。
3. 掌握 Rule of 3/5、移动语义、copy-and-swap。

## 知识点清单

- [ ] `start_` / `finish_` / `endOfStorage_` 三个指针的含义
- [ ] `size()` / `capacity()` 如何由指针差得到
- [ ] 默认构造、`vector(n)`、`vector(n, value)`、初始化列表（如有）
- [ ] `push_back(const T&)` 与 `push_back(T&&)` 两条路径
- [ ] 未满 / 已满两种情形：原地构造 vs `insert_aux`
- [ ] 扩容策略：`new_size = old_size * 2`
- [ ] 扩容时用 `uninitialized_copy` / 逐元素 move-construct
- [ ] 扩容失败：`destroy` 新空间 + `deallocate` + `throw`
- [ ] 拷贝构造与拷贝赋值（copy-and-swap）
- [ ] 移动构造 / 移动赋值与 `noexcept`
- [ ] `erase(iterator)` / `erase(first, last)`：搬移 + 析构尾部
- [ ] `insert(position, value)` / `insert(position, n, value)`
- [ ] `resize(n, value)` 的三条分支
- [ ] `at()` 越界检查与 `operator[]` 的区别
- [ ] `operator==` / `operator!=` 的单一真相源
- [ ] `clear()` / `swap()` / `pop_back()`
- [ ] 非 POD 类型、只可移动类型、会抛异常类型的行为

## 阅读顺序

1. 先读 [vector.h](../../../include/my_stl/containers/vector.h) 的成员声明，建立接口全貌
2. 读构造函数 / 析构函数 / `deallocate`
3. 读 `push_back` 与 `insert_aux`（先 const 引用版，再移动版，最后批量版）
4. 读 `erase` / `insert` / `resize`
5. 读拷贝/移动赋值与比较运算符
6. 对照 [test_vector.cpp](../../../tests/test_vector.cpp) 逐块确认行为

## 配套 Effective Modern C++

> 仓库：[../../../../Effective_Modern_Cpp](../../../../Effective_Modern_Cpp/README.md)，完整映射见 [reference-map.md](../reference-map.md)

- 基础 2：值类型与右值引用（`push_back` 两套重载的根源）
- 基础 10：构造函数语义学（拷贝/移动构造）
- **基础 11：异常处理与 swap & copy**（直接对应 copy-and-swap）
- 基础 17：RVO / NRVO（按值返回与移动构造）
- **条款 14：noexcept**（移动构造为什么必须标）
- 条款 17：理解特殊成员函数的生成（Rule of 3/5）
- **条款 23 / 24：std::move、forward、通用引用**（重载选择）
- 条款 1、5、6、7、13：推导、auto、初始化、const_iterator

实践方式：每学一条，就在 [vector_impl.h](../../../include/my_stl/detail/vector_impl.h)
里指出它对应的代码行；找不到对应实现时，记进笔记，判断是实现缺失还是条款不适用。

## 动手实验

- [ ] 打印每次扩容前后的 `size()` / `capacity()` / 地址，验证 2 倍增长
- [ ] 写一个只可移动、不可拷贝的类型，验证 `push_back(T&&)` 和扩容可用
- [ ] 写一个拷贝构造函数会抛异常的类型，在 `insert` 时触发异常，验证原 vector 不变且无泄漏（配合 ASan）
- [ ] 用 ASan 跑 `test_vector`，确认无 use-after-free / leak
- [ ] 手工模拟 `insert(position, value)` 在“未满”和“已满”两种情况下元素如何移动

## 自检问题

1. 为什么用三个指针而不是 `size_` + `capacity_` + `data_`？
2. 扩容为什么是 2 倍？如果每次只扩 1 个元素，复杂度会怎样？
3. 拷贝赋值为什么用 copy-and-swap？它提供了哪种异常保证？
4. 移动构造为什么标 `noexcept`？不标会有什么后果？
5. 扩容过程中如果第 k 个元素构造失败，已经构造的元素由谁析构？
6. `erase` 为什么不会缩容？`clear()` 会释放内存吗？
7. `insert_aux(position, const_reference)` 在容量已满时转调了哪个重载？这会影响只可移动类型吗？
8. `resize` 缩小和扩大分别做了什么？扩大时容量够和不够有什么区别？
9. `operator[]` 和 `at()` 的使用场景分别是什么？
10. 为什么 `operator!=` 直接返回 `!(*this == other)`？

## 学习中的疑问

<!-- 边学边记 -->
