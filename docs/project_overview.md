# Project Overview

本项目面向 STM32N647 智慧农业病虫害视觉诊断与闭环干预演示，覆盖端侧识别、蜂窝网络上报、云端接收、数据库落库和 Dashboard 可视化。

核心链路：

```text
STM32N647 -> L160/L610 -> Supabase Edge Function -> Supabase PostgreSQL -> Dashboard
```

本分支聚焦软件和云端交付，不合并硬件工程。硬件工程仍保留在 `codex/sensor-i2c-scan`、`codex/relay`、`codex/uart-printf` 等远端分支中。
