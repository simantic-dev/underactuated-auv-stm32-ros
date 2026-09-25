# 协议资料说明

本项目使用与匿名／ATKP 类协议相同的基础帧结构，但包含自定义消息字段和不同阶段的扩展，不能把第三方协议手册直接视为本项目的最终接口定义。

- 项目实际协议剖面见 [`auv-atkp-profile.md`](auv-atkp-profile.md)。
- STM32 实现位于 `COMMUNICATE/atpk.c`、`COMMUNICATE/atpk.h` 和 `COMMUNICATE/usblink.c`。
- ROS 解析与控制帧构造位于 `ros1_ws/src/serial_send_gps/src/`。
- 本地参考资料可放在 `docs/_local_reference/`；该目录已加入 `.gitignore`，不会随 `git add .` 上传。

当前本地参考文件为《匿名通信协议》V7.16（2022-03-21，共 26 页）。它是第三方资料，公开再发布前需要确认授权。
