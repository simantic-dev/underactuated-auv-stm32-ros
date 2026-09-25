# ROS 与 STM32 接口整理

本文记录本次将 Jetson／ROS 程序补充进欠驱动 AUV 主工程时确认的代码关系、版本边界和后续迁移顺序。

## 1. 采用的来源

| 来源 | 本次用途 |
| --- | --- |
| `zuizhong/01_STM32_H750_Firmware` | 与仓库中 STM32 固件核对接口 |
| `zuizhong/02_Jetson_ROS_Latest/src` | 作为 ROS 主线源码，复制两个 catkin 包 |
| `zuizhong/03_RViz_Map_Addons` | 提取地图点击选点和 GeoPoint 导引扩展 |
| `zuizhong/04_Test_Baseline_2025-04-03` | 作为历史快照比对，不重复收录 |
| `zuizhong/05_Test_Logs` | 仅作为存在日志的证据，不公开原始数据 |
| `sunyan.zip` | 比对多个历史工作空间；排除 `build/`、`devel/` 生成物 |

选择 `02_Jetson_ROS_Latest` 只是基于整理目录命名与已有分析，不等于它已被证明是最终上艇版本。

## 2. 设计数据流

```text
STM32 传感器与融合状态
  └─ ATKP 风格上行帧
       └─ ROS 串口接收／解析
            ├─ 当前定位与姿态话题
            ├─ CSV/文本日志
            └─ 航点导引节点
                 ├─ 期望航向／LOS 话题
                 ├─ RViz 航迹显示
                 └─ 0x41 下行帧 → STM32 控制接口
```

## 3. ROS 节点清单

### `serial_send_gps` 包

| CMake 目标 | 源文件 | 定位 |
| --- | --- | --- |
| `usb_datarecv_fixing1` | `usb_datarecv_fixing1.cpp` | 多消息 ID 串口解析、GPS/姿态发布和日志 |
| `usb_datarecv2` | `usb_datarecv2.cpp` | 另一版串口接收与 GPS 发布 |
| `los_s_result` | `los_s_result.cpp` | 主航点队列和期望航向计算 |
| `los_s_result_half` | `los_s_result_half.cpp` | 不同到达阈值／角度公式的阶段版本 |
| `serial_send_losdata` | `serial_send_losdata.cpp` | `/los` 到 `0x41` 串口帧 |
| `los_guidance_node` | `los_guidance_node.cpp` | 另一版导引实验 |
| `gps_data_input` | `gps_data_input.cpp` | 人工 GPS 输入／测试 |
| `yuan_gps_input` | `yuan_gps_input.cpp` | GPS 数据发生器／测试 |

`gps_path.cpp` 和 `test_node.cpp` 含路径显示代码，但当前包的 CMake 没有为它们创建目标。

### `serial_node` 包

该包保留了 `receiver.cpp`、`gps_data_receiver.cpp`、`gps_path.cpp`、`test_node.cpp` 和多个坐标转换实验，以及一组与 `serial_send_gps` 不同的自定义消息。它更接近早期实验集合，不应与主包的同名话题混合使用。

## 4. 消息与话题核对

| 话题 | 发布者 | 订阅者 | 已确认问题 |
| --- | --- | --- | --- |
| `current_gps` | `usb_datarecv_fixing1`：`serial_send_gps/usb_gps` | `los_s_result`：`serial_send_gps/gps` | 同名但消息类型不同，不能直接连接 |
| `/target_gps` | 外部输入 | `los_s_result` | 目标消息类型为 `serial_send_gps/gps` |
| `/start_cmd` | 操作员／外部节点 | `los_s_result` | `false` 时导引节点不处理当前位置 |
| `/los` | `los_s_result` | `serial_send_losdata` | 以 `gps.msg` 的 `los` 字段传递 |
| `/cmd_heading` | `los_s_result` | 未在保留主线中确认消费者 | 用四元数表达期望航向 |
| `/clicked_point` | RViz | `show_coordinate.py` | 输入 frame 未经 TF 统一 |
| `/geopoint` | `show_coordinate.py` | `los_s_result_geopoint` | 仅存在于实验扩展 |

建议后续统一使用标准消息：定位使用 `sensor_msgs/NavSatFix`，航向使用带时间戳的标量或姿态消息，目标点使用 `geographic_msgs/GeoPointStamped`；当前仓库暂不声称已经完成该迁移。

## 5. 上行协议差异

STM32 `COMMUNICATE` 模块和 ROS 接收程序均采用 `0xAA 0xFF` 帧头，但保留版本存在以下差异：

1. STM32 GPS 打包代码保存了纬度、经度各 `int32 × 10^7` 的版本；ROS `usb_datarecv_fixing1.cpp` 按经度、纬度和附加字段解析，并使用 `10^8` 缩放。
2. STM32 当前周期发送函数是否启用 GPS 打包，需要按实际调用路径确认。
3. ROS 解析器在每次串口 `read()` 内重新创建状态变量，跨 read 的半帧不能稳定保留。
4. ROS 中 SC/AC 校验代码被注释，不能拒绝损坏帧。
5. 姿态消息赋值连续覆盖 `msg.alt`，`rol`、`pit`、`yaw` 字段没有按定义分别写入。

## 6. 下行协议差异

`serial_send_losdata.cpp` 构造：

```text
AA FF 41 0E [7 × int16] SC AC
```

当前实现把 LOS×100 写入第一个 `int16`，只计算普通累加 `SC`，并把 `AC` 固定为 0。已读 STM32 `0x41` 解析版本从其他数据字节提取航向量，并检查双累加校验，因此不能直接认定两端已经对齐。

## 7. 坐标与导引边界

- 主线导引将当前经纬度和目标经纬度分别转换为 UTM，再计算两点方位角。
- 到达阈值在主线版本为 15 m，在 `half` 版本为 5 m；这些是程序判据，不是实测精度。
- 主线与 `half` 版本的角度变换公式不同，使用前需按艇体坐标系和航向定义统一。
- `show_coordinate.py` 固定使用 51N 区带，并直接读取点击点 x、y。
- `sunyan.zip` 中另一个脚本给点击点增加地图 UTM 原点偏移，两种做法对应不同 frame 假设，不能混用。
- `load_map.launch` 使用场景相关地图包名和静态 TF；本次没有复制地图栅格图片。

## 8. 推荐迁移顺序

1. 冻结一套实际 STM32 固件和一套 ROS 工作空间，保存版本号或提交号。
2. 以字节级协议表统一上行／下行消息 ID、字段偏移、类型、字节序、缩放和单位。
3. 提取可跨 `read()` 保留状态的流式解析器，并加入 SC/AC 校验。
4. 统一 `/current_gps` 消息类型，消除两个包的同名异型话题。
5. 将串口设备、波特率、日志路径、UTM 区带和到达阈值改为 ROS 参数。
6. 用录制帧或伪串口完成解析器和控制帧回环测试。
7. 在 RViz 中确认 map/utm/path TF 关系和地图原点。
8. 断开执行器动力完成 STM32—Jetson 台架联调。
9. 再进行受控场地／水池测试，单独记录传感器有效率、导引输出和到达结果。

## 9. 本次未做的事情

- 未修改历史 ROS 算法和协议实现。
- 未声称 `catkin_make` 或 Keil 重新编译通过。
- 未连接 Jetson、STM32、传感器或执行器。
- 未将日志中的坐标当作有效实艇轨迹或性能结果。
- 未复制 `sunyan.zip` 的生成目录和重复工作空间。
- 未复制场景相关的大幅地图图片。
