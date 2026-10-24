# MySTL 学习日志

记录从零重启 MySTL 的学习路线与笔记。

核心约定：**targets 是“学习前的地图”，notes 是“学习后的沉淀”**。目标文档完成即删除，笔记长期保留。

## 目录结构

```
doc/learning-log/
├── README.md          # 本文件：管理规则 + 状态索引
├── targets/           # 待学习目标（学完即删）
├── notes/             # 学习笔记（长期保留）
├── templates/         # 目标文档 / 笔记模板
└── reference-map.md   # Effective Modern C++ × MySTL 对照与实战指南
```

配套仓库：[../Effective_Modern_Cpp](../../../Effective_Modern_Cpp/README.md)（现代 C++ 规则与示例）。
每个阶段的 EMCPP 对应内容、实战动作和实验代码组织方式见
[reference-map.md](reference-map.md)。

## 工作流

1. 从 `targets/` 里取当前阶段的目标文档，按「知识点清单」学习。
2. 边学边在目标文档上打勾、记疑问，不用另开文件。
3. 学完后，复制 [templates/note-template.md](templates/note-template.md) 到 `notes/`，**编号和主题与目标文档保持一致**。
4. 把目标文档里的收获、疑问解答、实验结果整理进笔记。
5. 删除 `targets/` 里对应的目标文档。
6. 更新下面状态表中的「笔记」链接和状态。

> 编号规则：`NN-topic.md`。targets 和 notes 共用同一编号，方便追溯。
> 命名统一用小写英文 + 连字符，避免跨平台路径问题。

## 状态索引

| 编号 | 主题 | 目标文档 | 笔记 | 状态 |
|------|------|----------|------|------|
| 01 | C++ 基础与对象生命周期 | — | [01-cpp-basics.md](notes/01-cpp-basics.md) | ✅ 已完成 |
| 02 | 空间配置器 alloc | — | [02-allocator.md](notes/02-allocator.md) | ✅ 已完成 |
| 03 | 模板萃取 / 迭代器 / 构造与算法 | [03-traits-iterators.md](targets/03-traits-iterators.md) | — | ⬜ 待开始 |
| 04 | vector 完整实现 | [04-vector.md](targets/04-vector.md) | — | ⬜ 待开始 |
| 05 | list | — | — | ⏸ 待规划 |
| 06 | skip_list 与并发 | — | — | ⏸ 待规划 |

状态：⬜ 待开始 / 🟡 进行中 / ✅ 已完成 / ⏸ 暂缓

## 维护建议

- 每个主题只保留**一份**目标文档和**一份**笔记，不按日期堆文件。
- 学习周期较长时，可以在目标文档顶部写「最后更新」日期。
- 笔记里贴代码时用 `file:line` 形式标注出处，例如 `include/my_stl/core/alloc.h:150`。
- 学完一个阶段就在 README 状态表里更新，避免时间久了忘记进度。
