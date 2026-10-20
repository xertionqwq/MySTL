# 笔记：C++ 基础与对象生命周期

- 编号：01
- 完成日期：2026-10-20
- 对应源码：[construct.h](../../../include/my_stl/core/construct.h)、[allocator.h](../../../include/my_stl/core/allocator.h)
- 实验代码：[experiments/01-topic/experiment.cpp](../experiments/01-topic/experiment.cpp)
- 原目标文档：已删除（内容已整理进本笔记）

## 一句话总结

内存分配和对象构造是两件事：分配只关心字节数和对齐，构造才关心类型和值。把二者拆开，才能在同一块内存上反复构造/析构、按类型特征决定析构策略，并在异常时精确回滚。

## 核心概念

- **栈 / 堆 / 静态存储区**：栈对象离开作用域自动析构（作用域决定生命周期）；堆对象要手动 `free`/`delete`；静态对象程序结束才析构。
- **`malloc/free`**：只分配/回收字节，不调用构造/析构，返回 `void*`（不携带类型信息）。失败返回 `nullptr`。
- **`new/delete`**：`new T` = `operator new`（分配）+ 构造函数（构造）；`delete` = 析构 + `operator delete`（释放）。分配失败抛 `std::bad_alloc`。
- **placement new**：`new (ptr) T(args...)`，只构造不分配，ptr 必须是已存在的、对齐正确、未构造的内存。
- **显式析构**：`ptr->~T()`，只析构不释放，之后该内存可以再次 placement new。
- **POD / trivial 类型**：析构无副作用，可跳过析构、可 `memcpy`。这是 `destroy` 标签分派优化的前提。
- **构造函数抛异常**：已构造完成的成员（按构造反序）会被自动析构，但对象本身不完整、其析构函数**不会**执行。
- **RAII**：资源获取即初始化，析构函数自动释放，异常栈展开时保证清理。
- **栈展开**：异常向上传播时，沿途所有已构造对象的析构函数按作用域逆序执行。
- **`noexcept`**：声明函数不抛异常；若标了却真抛出，直接 `std::terminate`。移动构造标 `noexcept` 是后续 vector 扩容走移动的关键。

## 关键代码路径

- [construct.h:14-17](../../../include/my_stl/core/construct.h#L14-L17) `construct(T1*, const T2&)`：拷贝/转换构造，源类型可不同
- [construct.h:20-24](../../../include/my_stl/core/construct.h#L20-L24) `construct(T*, T&&)`：移动构造，仅同类型
- [construct.h:27-31](../../../include/my_stl/core/construct.h#L27-L31) `destroy(T*)`：单对象显式析构
- [construct.h:33-48](../../../include/my_stl/core/construct.h#L33-L48) `destroy(first, last)`：按 `is_POD_type` 标签分派，POD 走空操作
- [allocator.h:52-59](../../../include/my_stl/core/allocator.h#L52-L59) `allocate()` / `allocate(n)`：只拿字节，返回 `T*`
- [allocator.h:72-90](../../../include/my_stl/core/allocator.h#L72-L90) `construct` / `destroy`：委托给全局 `MySTL::construct` / `destroy`

## 实验与验证

实验文件 [experiment.cpp](../experiments/01-topic/experiment.cpp)，编译命令：

```bash
clang++ -std=c++17 -Wall -Wextra -g -fsanitize=address -I include \
    doc/learning-log/experiments/01-topic/experiment.cpp -o /tmp/mystl_exp01
```

- **实验一（分离）**：`malloc(sizeof(A))` → `static_cast<A*>` → `construct` → `destroy` → `free`。确认四步各司其职，`live` 与栈上 `aa` 是两个对象（各自析构一次）。
- **实验二（反复构造）**：同一地址 `0x...0030` 上循环 5 次 construct/destroy，地址始终不变。注意 `construct(live, A(i))` 传临时对象，命中 `T&&` 移动重载。
- **实验三（构造抛异常 + 栈展开）**：`B(1e6)` 构造中抛出，输出确认析构顺序为 `~A(91)`（B 的成员）→ `~A(99)`（栈上 a1），即**构造反序**。

结论均与理论一致，`-Wall -Wextra` 零警告，ASan 无泄漏/越界报告。

## 踩过的坑与疑问解答

### 1. 为什么拷贝用 `T1`/`T2`，移动统一成 `T`？

- 拷贝版本 `construct(T1* ptr, const T2& value)` 的源类型 `T2` 可以和目标 `T1` **不同**：`new (ptr) T1(value)` 允许用 `const char*` 构造 `std::string`、用 `Derived` 构造 `Base`（SGI STL 原始设计）。
- 移动版本只能同一 `T`，因为"移动"的本质是窃取资源，只发生在完全同类型之间，从不同类型"移动"语义不成立。
- 不用一个 `construct(T1*, T2&&)` 通吃，是因为转发引用 `T2&&` 会贪婪匹配左值，无法区分"拷贝还是移动"。拆成 `const T2&`（左值/不同类型）+ `T&&`（同类型右值）两重载，意图才清晰——与 `push_back(const T&)` / `push_back(T&&)` 同思路。

### 2. 为什么不能传 `void*`，要先强转成类指针？

`construct` 是模板，`T1` 从指针实参类型推导。传 `void*` 会推导出 `T1 = void`，`new (ptr) void(...)` 非法（`void` 无大小、无法构造）。指针类型**承载了"要构造什么"的类型信息**，placement new 靠它决定调用哪个构造函数、占多大、怎么对齐。`malloc` 返回 `void*` 正是因为只负责字节、不关心类型；强转由知道"这里放什么"的调用方在构造时刻补回类型。`static_cast<A*>(void*)` 安全，因为 `malloc` 保证返回内存对任意对象类型对齐。

### 3. 实验三原本 rethrow 逃出 `main`

原 `catch (...) { throw; }` 让异常逃出 `main` → `std::terminate` → abort（退出码 134），`"return done"` 不打印。已改为就地 `catch (const std::exception& e)` 打印后继续。

### 4. `B::buf` 未初始化

构造体内 `throw` 发生在 `buf = new int[size]` 之前，失败对象不完整、`~B()` 不执行，所以侥幸安全；但这是悬垂指针隐患。已用成员初始化列表 `buf(nullptr)` 置空。

### 5. 移动重载实际退化为拷贝

实验二命中了 `construct(T*, T&&)`，但 `A` 自定义了析构函数 → 隐式移动构造被抑制 → `std::move(value)` 最终绑到 `A(const A&)`，实际是拷贝。这正是 EMCPP 条款 17「特殊成员函数生成」的活案例。

## 自检问题答案

1. **`new T` vs `malloc + placement new`**：`new T` = 分配 + 构造，分配失败抛 `bad_alloc`，构造失败时语言会自动调用 `operator delete` 回收内存；`malloc + placement new` 分配失败返回 `nullptr`（不抛），构造失败时**内存不会自动回收**，必须自己 `free`——这正是 vector 扩容代码里 try/catch 调 `deallocate` 回滚的原因。

2. **只 `free` 不析构会漏什么**：漏掉对象**内部持有的资源**（内部动态内存、文件句柄、非平凡成员如 `std::string` 的缓冲区等）。`free` 只回收对象自身字节，析构函数从不执行，内部资源全部泄漏。

3. **为什么拆开 `allocate` / `construct`**：分配是"字节 + 对齐"层面的事，构造是"类型 + 值"层面的事。拆开后才能：① 分配一次、反复构造/析构（复用内存，如 vector 的池）；② 批量分配 n 个元素再逐个/懒构造；③ 让配置器在字节层面与类型解耦（`alloc` 只管字节，`allocator<T>` 才带类型）。

4. **构造抛异常谁回收内存**：`new T` 由语言自动 `operator delete` 回收；`malloc + placement new` 无人回收，裸内存泄漏，除非在 `catch` 里 `free`。异常安全的代码必须在构造块外包 try/catch 并释放。

5. **trivial 析构为什么可以什么都不做**：这类类型（内置类型、全 trivial 成员的聚合）析构没有副作用，逐个调用 `~int()` 是纯浪费。通过类型特征（`is_POD_type` / `_true_type`）在编译期分派：POD 走空操作，非 POD 逐个析构——既正确又零开销。

## 仍然不确定的地方

- `construct(T1*, const T2&)` 跨类型构造除了 `const char* → std::string` 还有哪些实际场景？（留待用 `list` / 实际容器时观察）
- 移动重载为什么不用转发引用 `T&&` 做完美转发，而是直接 `std::move`？二者对 `const` 右值和值类别的处理有何差异？（留待阶段 03 条款 23/24 深究）
