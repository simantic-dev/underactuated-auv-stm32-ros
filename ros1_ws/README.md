# Jetson ROS 1 source workspace

本目录保存欠驱动 AUV 的 Jetson／ROS 1 侧程序。源码主线来自 `zuizhong/02_Jetson_ROS_Latest/src`；`sunyan.zip` 中存在多个重叠工作空间和大量 `build/`、`devel/` 生成文件，因此没有整包复制。

> 这是保留源码的工程化整理，不是已经重新验证的一键运行版本。本轮没有在 Jetson 或 ROS 环境中执行 `catkin_make`，也没有连接 STM32、推进器或实艇。

## 目录

```text
ros1_ws/
├─ src/
│  ├─ serial_send_gps/    # 主要串口、导引、下发与日志程序
│  └─ serial_node/        # 早期串口、消息和轨迹实验程序
└─ experimental/
   └─ rviz_map/           # 地图点击选点和 GeoPoint 导引扩展
```

`serial_send_gps` 的 CMake 当前生成 8 个 C++ 可执行目标，但它们是不同阶段的测试和功能入口，不应全部同时启动。`serial_node` 也保留了多个早期实验节点。

## 核心文件

| 文件 | 作用 | 当前边界 |
| --- | --- | --- |
| `serial_send_gps/src/usb_datarecv_fixing1.cpp` | 解析 STM32 上行帧、发布 `current_gps`、写日志 | 发布类型为 `usb_gps`；校验被注释，姿态字段写入有覆盖问题 |
| `serial_send_gps/src/usb_datarecv2.cpp` | 另一版 GPS 串口解析 | 设备名和协议 ID 与其他版本不同 |
| `serial_send_gps/src/los_s_result.cpp` | 航点队列、UTM 转换、距离判定和期望航向 | 订阅类型为 `gps`，与 fixing1 版本不直接兼容 |
| `serial_send_gps/src/serial_send_losdata.cpp` | 将 `/los` 封装为 `0x41` 控制帧 | `ADD_SUM` 固定为 0，字段位置需与 STM32 对齐 |
| `serial_send_gps/src/gps_path.cpp` | GPS 相对轨迹、TF 和 `nav_msgs/Path` | 当前 CMake 未生成该目标 |
| `serial_node/launch/gps_active.launch` | 历史组合启动文件 | 引用了当前未启用或版本不同的节点 |
| `experimental/rviz_map/scripts/show_coordinate.py` | `/clicked_point` 转 `/geopoint` | 假设固定 UTM 区带，未处理任意输入 frame |
| `experimental/rviz_map/src/los_s_result_geopoint.cpp` | 以 `/geopoint` 作为目标输入 | 尚未合并到主包 CMake |

## 话题关系

```text
STM32 serial
    │
    ├─ usb_datarecv_fixing1 / usb_datarecv2
    │          └─ current_gps
    │                    │
target_gps ──────────────┤
start_cmd ───────────────┤
                         v
                   los_s_result
                  ├─ cmd_heading
                  └─ los ──> serial_send_losdata ──> STM32 serial

RViz clicked_point ──> show_coordinate.py ──> geopoint
                                             └─> los_s_result_geopoint（实验）
```

这张图表示设计关系，并不说明当前各文件的消息类型和协议已经全部一致。

## 编译准备

工作空间保留 ROS Noetic 的路径痕迹，实际使用前请按目标 Jetson 系统确认 ROS 发行版。基础构建命令为：

```bash
cd ros1_ws
catkin_make
source devel/setup.bash
```

依赖包括：

- `roscpp`、`rospy`、`std_msgs`
- `serial`
- `tf2`、`tf2_ros`、`tf2_geometry_msgs`
- `geometry_msgs`、`nav_msgs`、`visualization_msgs`
- `geographic_msgs`、`geodesy`
- GeographicLib

由于本轮没有 ROS 环境，未声明 `catkin_make` 已通过。如果构建失败，应先核对 `package.xml` 的依赖声明、消息生成依赖和 CMake 中已启用的目标，而不是直接修改协议数据。

## 安全联调

1. 断开执行器动力。
2. 确认 `/dev/ttyUSB0`、`/dev/ttyACM0` 实际对应的设备。
3. 先只启动接收节点并检查 ROS 消息。
4. 使用离线或人工数据测试导引节点。
5. 对齐 `0x41` 字段、缩放和 SC/AC 双校验。
6. 完成串口回环测试后，再在拆桨条件下启用下发节点。

详细接口审计见 [`../docs/ros-integration.md`](../docs/ros-integration.md)。

