# ROS 历史源码归档

当前可维护主线位于仓库根目录的 `ros1_ws/src/`。本目录只保存 `sunyan.zip` 中四个历史工作空间的源码树，目的是防止阶段性节点、消息和脚本在整理时丢失。

| 历史工作空间 | 主要内容 |
| --- | --- |
| `serial_node_ws` | 早期串口接收、坐标转换、轨迹显示和自定义消息 |
| `serial_send` | 串口发送、GPS/LOS 多个阶段版本和辅助包 |
| `stm32_to_ros_ws` | STM32 到 ROS 的早期话题示例与串口代码 |
| `usb_ros_ws` | USB 串口接收、GPS/LOS 和日志阶段版本 |

## 使用边界

- 四个工作空间中存在同名包、同名节点和接口不一致的历史版本，不能同时加入一个 catkin 工作空间编译。
- `build/`、`devel/` 和其他自动生成目录没有复制，因为它们不是源代码，可由对应环境重新生成。
- ZIP 中的工作空间顶层 `src/CMakeLists.txt` 是指向 ROS Noetic catkin 顶层文件的符号链接。Windows 解包无法创建该 Linux 符号链接，因此这里以记录链接目标的一行文本保留；实际使用时应在 Linux/ROS 环境中重新执行 `catkin_init_workspace`。
- 需要恢复某一历史版本时，应单独复制对应工作空间，不要覆盖当前 `ros1_ws/src` 主线。
