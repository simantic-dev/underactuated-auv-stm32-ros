# STM32 版本保留说明

当前仓库根目录的 STM32 工程以 `zuizhong/01_STM32_H750_Firmware` 作为上传主线。

在最终合并前，下载目录中的工程与该主线绝大多数源码仅存在换行差异；实际内容差异集中在：

- `Core/Src/main.c`：旧文件把 `myQueue01Handle` 的外部声明误写成 `myQueue01Handl`。
- `HARDWARE/imu.c`：旧文件少一行说明性注释，不影响执行逻辑。
- 根目录缺少 `.clangd` 与 `compile_flags.txt` 开发配置。

为保证不丢失任何历史内容，合并前的两个文件原样保存在 `pre-final-merge/`。根目录主线使用整理来源中的对应版本，并补回两个开发配置文件。

本目录是历史比对材料，不应加入 Keil 工程或参与固件编译。
