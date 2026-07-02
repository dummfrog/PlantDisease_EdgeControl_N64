# Firmware Branch Notes

STM32 工程目前主要位于远端硬件分支：

- `codex/sensor-i2c-scan`
- `codex/relay`
- `codex/uart-printf`

本次软件分支不合并、不覆盖硬件工程，只保留本说明文件用于指向硬件分支。

后续如需集成固件，建议单独创建硬件集成分支并审查 STM32CubeIDE、`Appli/Core`、`Secure_nsclib` 和 `.ioc` 等工程文件。
