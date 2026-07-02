# Hardware Integration Summary

本软件分支不合并 STM32 工程，避免覆盖硬件同学已经上传的工作。

当前硬件相关远端分支：

- `codex/sensor-i2c-scan`
- `codex/relay`
- `codex/uart-printf`

建议后续新建单独硬件集成分支，从上述分支中选择基线并逐项合并 UART、继电器、传感器、泵、风扇和上报链路。
