# “海洋探索者”AUV代码汇总与来源说明

整理时间：2026-09-04

本目录中的程序均从原项目目录原样复制。为避免破坏历史证据，本次只做分类和重命名，没有修改源代码内容。

## 目录结构与来源

### 01_STM32_H750_Firmware

- 原始位置：`C:\Users\lxx\Desktop\项目经历\1\IM-7-3\IM-7-3\IM\IMUDM\IMUDMA_New`
- 内容：STM32H750VBT6、FreeRTOS、UART+DMA、USB CDC、GPS、Xsens IMU、DVL、深度计、执行器控制、PID/EKF等固件代码及Keil/CubeMX工程文件。
- 未复制生成文件：`Debug`、`MDK-ARM\IMUDMA`、`.vscode`。
- 判断：当前提供资料中唯一且最新的STM32完整源码工程。

### 02_Jetson_ROS_Latest

- 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\usb_ros_ws\src`
- 内容：USB/串口数据接收、ROS消息、LOS计算、控制帧发送等Jetson端ROS源码。
- 判断：与2025-04-03测试快照相比，仅`usb_gps.msg`和`usb_datarecv_fixing1.cpp`有后续修改，因此作为Jetson端“最新修改主线”。

### 03_RViz_Map_Addons

这些功能没有完整保留在最新`usb_ros_ws/src`中，因此从较早但功能更完整的`serial_send`工作空间单独补充：

- `scripts/show_coordinate.py`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\serial_send\src\serial_send_gps\scripts\show_coordinate.py`
  - 用途：订阅RViz的`/clicked_point`，把UTM点转换为经纬度并发布`/geopoint`。
- `launch/load_map.launch`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\serial_send\src\serial_send_gps\launch\load_map.launch`
  - 用途：加载地图并发布UTM到map的静态TF。
- `map/`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\serial_send\src\serial_send_gps\map`
  - 用途：实测地图、地图配置和辅助文件。
- `src/los_s_result_geopoint.cpp`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\serial_send\src\serial_send_gps\src\los_s_result.cpp`
  - 用途：接收`/geopoint`目标点并完成UTM/LOS处理。复制时仅改了文件名，源代码内容未改。

### 04_Test_Baseline_2025-04-03

- 原始源码位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\usb_ros_ws\2025.4.3CEISHI\src`
- 原始压缩包：`C:\Users\lxx\Desktop\项目经历\4\sunyan\usb_ros_ws\2025.4.3CEISHI.zip`
- 判断：压缩包中文件的最后修改时间为2025-04-03 11:15左右；同目录LOS日志开始于2025-04-03 11:11，因此这是最可信的当天测试快照。

### 05_Test_Logs

- `usb_ros_ws_gps_data.txt`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\usb_ros_ws\gps_data.txt`
  - 内容：2025-04-03 17:30:56至17:35:55的10 Hz数据记录，共约963行。
- `usb_ros_ws_los_data.csv`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\usb_ros_ws\los_data.csv`
  - 内容：2025-04-03 11:11左右的LOS发送记录。
- `serial_send_gps_data.txt`
  - 原始位置：`C:\Users\lxx\Desktop\项目经历\4\sunyan\serial_send\gps_data.txt`
  - 内容：用于GPS航迹回放/验证的数据。

## 未纳入最终代码的目录

- `serial_node_ws`：2025年3月早期GPS接收、坐标转换和航迹显示原型，相关最终功能已在后续工程或补充目录中保留。
- `stm32_to_ros_ws`：轮式机器人串口通信参考例程，不属于该AUV最终业务代码。
- `build`、`devel`：ROS编译产物，不是源码，未复制。

## 当前代码的重要注意事项

1. `02_Jetson_ROS_Latest`是最新修改版，不代表已经完全验收。`usb_datarecv_fixing1.cpp`中横滚、俯仰、航向目前连续写入了`msg.alt`，后续应分别写入`msg.rol`、`msg.pit`、`msg.yaw`。
2. `serial_send_losdata.cpp`只计算了SC校验，AC附加校验仍为0；同时LOS写入`ctrl_rol`，而STM32的`0x41`处理逻辑读取的是`ctrl_yawdps`对应位置。进行闭环联调前需要统一协议。
3. `03_RViz_Map_Addons`是从较早版本补充的功能代码，尚未与最新ROS主线正式合并。
4. 旧`gps_active.launch`引用了部分已从CMake构建列表中注释的节点，后续应重新制作统一启动文件。

建议后续以`02_Jetson_ROS_Latest`为整理主线，以`04_Test_Baseline_2025-04-03`作为历史实测对照，不直接修改测试快照。
