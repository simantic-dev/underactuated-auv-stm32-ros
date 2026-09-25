# STM32H750 AUV 控制板接口映射

本文根据 `stm32h750-auv-controller-v1.pdf` 与当前 STM32 工程进行静态核对，用于说明原理图、CubeMX 配置和固件调用之间的关系。原理图没有完整标题栏和硬件修订号，因此以下内容不能替代实艇接线表。

## 已确认对应关系

| 模块／接口 | 原理图连接 | 固件证据 | 结论 |
| --- | --- | --- | --- |
| 主控 | STM32H750VBT6，LQFP100 | `IMUDMA.ioc` 指定 STM32H750VBTx | 一致 |
| DVL-A50 | USART1，独立 12 V / 3 A 电源 | `DVL/DVL.c` 使用 `huart1` 和 Receive-to-Idle DMA | 一致 |
| MTi-630 | UART4，并预留 CAN、GNSS、同步信号 | `HARDWARE/imu.c` 使用 `huart4` | 串口主链路一致 |
| Bar30 | UART8，5 V 供电 | `HARDWARE/deph.c` 使用 `huart8` | 一致 |
| Ping2 Sonar | USART2，5 V 供电 | `COMMUNICATE/Uart2link.c` 使用 `huart2` | 一致 |
| 板载 IMU | BMI088，SPI1 | 原理图给出独立加速度计／陀螺仪片选 | 硬件存在，实际启用版本需复核 |
| 外接 IMU | ICM-42688 风格 SPI 接口与加热控制 | 原理图给出 SPI、INT 和 `PWR_HEAT` | 需结合实际外接模块确认 |
| USB | USB Type-C，USB OTG FS | 工程启用 USB Device CDC | 可作为 Jetson／上位机虚拟串口 |
| 执行机构 | TIM1、TIM8 PWM 与串口输出 | `moter/moter.c` 同时保留定时器 PWM 和 USART3 文本输出 | 存在多个阶段实现 |

## 电源与外设供电

- 主电源输入标注为 14–28 V。
- 原理图包含 24 V、12 V、10 V、5 V 和 3.3 V 等电源域。
- DVL-A50 支路标注为 12 V / 3 A。
- Jetson Nano 支路标注为 5 V / 3 A。
- 推进／执行支路包含较大电流器件，实际上电前应核对 MOSFET、采样电阻、电感、接插件和散热能力。

## 需要在实艇上复核的接口

### STM32 与 Jetson

原理图将 USART3 标为 Jetson Nano 串口，但当前 `moter/moter.c` 会通过 `huart3` 发送如下文本：

```text
PWM1:...,PWM2:...,PWM3:...,PWM4:...
```

与此同时，ATKP 遥测代码默认调用 USB CDC，ROS 历史节点又分别配置过 `/dev/ttyACM0` 和 `/dev/ttyUSB0`。因此仓库目前只能确认存在以下接口方案，不能断言哪一种是最终上艇版本：

1. USB CDC：STM32 与 Jetson 直接交换 ATKP 风格二进制帧。
2. USART3：STM32 向 Jetson 或下级设备发送 PWM 文本。
3. USB 转串口：ROS 通过 `/dev/ttyUSB0` 接入其他串口链路。

### PWM 输出

原理图给出多路 PWM 接口；当前 `Control_PWM_Set()` 的有效调用路径包括推进输出和 USART3 舵机文本输出，部分 TIM1 舵机 PWM 调用处于注释状态。公开文档应区分“硬件已预留”和“当前固件已启用”。

## 使用建议

在后续台架联调前，建议生成一张带硬件修订号的最终接线表，至少记录：设备、连接器编号、串口实例、引脚、波特率、供电电压、最大电流、协议版本和实际线束颜色。
