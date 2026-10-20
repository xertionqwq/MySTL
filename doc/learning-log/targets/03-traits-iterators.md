# 目标：模板萃取、迭代器与批量构造算法

- 编号：03
- 对应源码：[typeTraits.h](../../../include/my_stl/core/typeTraits.h)、[iterator.h](../../../include/my_stl/core/iterator.h)、[construct.h](../../../include/my_stl/core/construct.h)、[allocator.h](../../../include/my_stl/core/allocator.h)、[algorithm.h](../../../include/my_stl/core/algorithm.h)、[uninitialized.h](../../../include/my_stl/core/uninitialized.h)
- 前置知识：阶段 01、02；模板基本语法
- 开始日期：
- 完成标准：能解释 `uninitialized_copy` 为什么要分成 `_true_type` / `_false_type` 两条路径，并说明异常时如何回滚

## 学习目标

1. 掌握全特化、偏特化、标签分派这三种本项目用到的模板技术。
2. 理解迭代器分类与 `iterator_traits` 的作用。
3. 理解“分配与构造分离”在 `allocator<T>` 和 `uninitialized_*` 中的落地。

## 知识点清单

- [ ] 类模板、成员模板、模板默认参数
- [ ] 全特化（每个内置类型的 `_type_traits`）与偏特化（`IfThenElse<bool, Ta, Tb>`）
- [ ] 标签分派：`_true_type{}` / `_false_type{}` 选择重载
- [ ] 依赖名与 `typename`：`typename iterator_traits<It>::value_type`
- [ ] 嵌套模板消歧义：`typename Alloc::template rebind<U>::other`
- [ ] 迭代器五种标签及其继承关系（自动降级）
- [ ] `iterator_traits`：从迭代器提取 `value_type` / `difference_type` 等
- [ ] 裸指针作为随机访问迭代器
- [ ] `allocator<T>` 的 `allocate` / `construct` / `destroy` 分离
- [ ] `rebind` 的作用：为节点类型分配内存
- [ ] `uninitialized_copy` / `uninitialized_fill_n`：在原始内存上构造
- [ ] 批量构造失败时的回滚：`destroy(result, cur); throw;`
- [ ] `copy` / `copy_backward` 的区间方向

## 阅读顺序

1. [typeTraits.h](../../../include/my_stl/core/typeTraits.h)：泛型版本 + `int` 特化 + `IfThenElse`，**跳过其余重复特化**
2. [iterator.h](../../../include/my_stl/core/iterator.h)：五种 tag、`iterator` 基类、`iterator_traits`
3. [construct.h](../../../include/my_stl/core/construct.h)：标签分派的第一个实例
4. [allocator.h](../../../include/my_stl/core/allocator.h)：`rebind` 与静态成员函数
5. [algorithm.h](../../../include/my_stl/core/algorithm.h)：`copy` / `copy_backward` / `fill_n`
6. [uninitialized.h](../../../include/my_stl/core/uninitialized.h)：分派 + 回滚

## 配套 Effective Modern C++

> 仓库：[../../../../Effective_Modern_Cpp](../../../../Effective_Modern_Cpp/README.md)，完整映射见 [reference-map.md](../reference-map.md)

- **条款 1：理解模板类型推导**（读 `_type_traits` 和 `allocator<T>` 之前必看）
- 条款 2 / 3：auto 与 decltype 类型推导
- 条款 9：优先使用别名声明而非 typedef
- 条款 13：优先使用 const_iterator（为 vector/list 的 const 重载做铺垫）
- 条款 15：尽可能使用 constexpr
- 基础 3：数组、函数与指针（裸指针为什么能当迭代器）
- 基础 8：可调用对象类型（后续 `for_each` 回调会用到）
- 基础 9：CRTP 与编译期分派思想

注意：EMCPP 仓库不覆盖模板偏特化和 traits 本身，这部分以
[typeTraits.h](../../../include/my_stl/core/typeTraits.h) 和《STL 源码剖析》为主。

## 动手实验

- [ ] 用 `static_assert` 检查 `_type_traits<int>::is_POD_type` 与自定义结构体的差异
- [ ] 给自定义类型特化 `_type_traits`，观察 `destroy` 走哪条路径
- [ ] 写一个最小自定义迭代器（只需 `value_type` 等五个类型别名），传给 `uninitialized_copy`
- [ ] 让拷贝构造函数在第二次调用时抛异常，统计成功构造了几个、析构了几个，验证回滚
- [ ] 打印 `_true_type` / `_false_type` 两条路径分别被哪个重载处理

## 自检问题

1. 为什么用 `_true_type` / `_false_type` 两个类型，而不是给模板传一个 `bool`？
2. 对 POD 类型批量析构为什么可以什么都不做？这依赖什么前提？
3. `uninitialized_copy` 里 `catch (...)` 为什么调用 `destroy(result, cur)` 而不是 `destroy(result, last)`？
4. `allocator<T>::construct` 为什么有 `const T&` 和 `T&&` 两个重载？
5. `rebind` 解决了什么问题？不用它，`list` 的节点内存怎么分配？
6. 为什么 `forward_iterator_tag` 要继承 `input_iterator_tag`？这对重载选择有什么影响？
7. `copy_backward` 和 `copy` 分别用于什么场景？重叠区间时有什么要求？

## 学习中的疑问

<!-- 边学边记 -->
