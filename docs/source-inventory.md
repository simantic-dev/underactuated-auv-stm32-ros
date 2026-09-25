# 源码来源与完整性清单

本文档记录最终上传前的静态核对结果。“已核对”表示文件路径和内容已比较，不等于本轮已重新编译或实机验证。

| 原始材料 | 仓库位置 | 静态核对结果 | 处理说明 |
| --- | --- | --- | --- |
| `zuizhong/01_STM32_H750_Firmware` | 仓库根目录 STM32 工程 | 217 个源文件/配置路径已覆盖 | 实质差异文件先归档后同步；换行符差异不视为代码删减 |
| `zuizhong/02_Jetson_ROS_Latest` | `ros1_ws/src` | 35/35 个文件路径已保留 | 根 `CMakeLists.txt` 使用可提交的 catkin 顶层文件，代替原 Linux 绝对路径软链接 |
| `zuizhong/03_RViz_Map_Addons` | `ros1_ws/experimental/rviz_map` | 9/9 个文件已保留 | 包括 launch、YAML、Python/C++ 节点和 5 个地图栅格/辅助文件 |
| `zuizhong/04_Test_Baseline_2025-04-03` | `archive/ros_baseline/2025-04-03` | 35 个 `src` 文件及原始备份包已保留 | 与主线完全分开，用于对比历史报文和节点行为 |
| `sunyan.zip` | `archive/ros_history` | 4 个历史 `src` 树、119 个文件已分开归档 | 未复制 `build/`、`devel/`等生成物；Linux `src/CMakeLists.txt` 软链接以其目标文本记录 |
| `zuizhong/05_Test_Logs` | 未纳入 Git 源码树 | 不属于程序代码 | 原始日志和测试数据保持在用户原目录，本轮未删除、改写或当作性能证据上传 |

## 版本边界

- 主线代码是建议首先阅读和编译的版本；`archive/` 是为了防止旧代码在整理时丢失。
- 历史 ROS 工作空间之间存在同名包、同名消息和不同协议字段，应在独立工作空间里分别验证。
- 本清单不声称这些版本能在当前 Windows 环境中同时编译，也不声称已重新完成湖试或整艇联调。
- 原始源目录及压缩包未被本轮整理删除。

## 上传后核对

`SOURCE_MANIFEST.sha256` 列出了整理完成时拟被 Git 跟踪文件的 SHA-256。在另一台计算机 clone 仓库后，可根据该文件检查上传过程是否改变了文件内容。
