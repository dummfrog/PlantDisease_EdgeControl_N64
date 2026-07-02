# Git Remote Status Report

生成时间：2026-07-02

## Remote URL

```text
origin https://github.com/dummfrog/PlantDisease_EdgeControl_N64.git
```

## 当前本地分支

```text
software/cloud-edge-dashboard
```

本地分支从 `origin/main` 创建，创建前 `main` 已执行 `git pull origin main`，结果为 up to date。

## 远端分支列表

```text
origin/main
origin/codex/relay
origin/codex/sensor-i2c-scan
origin/codex/uart-printf
```

远端 tag：无。

## 每个远端分支最近 commit

```text
origin/main                  f08286b 2026-06-19 00:11:58 +0800 Initial commit
origin/codex/relay           7511804 2026-06-21 19:05:52 +0800 feat(relay): verify PF11 PF12 relay GPIO control
origin/codex/sensor-i2c-scan 093bd68 2026-06-29 02:30:55 +0800 feat(sensor): restore INA219 current monitoring
origin/codex/uart-printf     0d5662b 2026-06-19 22:06:53 +0800 test: verify UART heartbeat on N647
```

## main 当前文件概览

```text
.gitattributes
```

## codex/sensor-i2c-scan 文件概览

该分支包含完整 STM32 工程结构和传感器相关代码，代表当前最完整的硬件工作之一：

```text
.gitignore
.mxproject
.project
01_LED.ioc
AGENTS.md
Appli/Core/Inc/
Appli/Core/Src/
PROJECT_CONTEXT.md
ROADMAP.md
STM32CubeIDE/
Secure_nsclib/
TOMORROW_RELAY_TEST_CHECKLIST.md
```

重点文件包括 `bh1750`、`bmp280`、`ds3231`、`ina219`、`relay`、`sensor`、`pump`、`fan`、`log_upload`、`prescription` 和 `ai_result` 相关源文件。

## codex/relay 文件概览

该分支包含继电器验证相关 STM32 工程：

```text
.gitignore
.mxproject
.project
01_LED.ioc
AGENTS.md
Appli/Core/Inc/
Appli/Core/Src/
PROJECT_CONTEXT.md
STM32CubeIDE/
Secure_nsclib/
```

重点文件包括 `relay.c`、`relay.h`、`buzzer`、`app_uart` 和基础 `main` 工程文件。

## codex/uart-printf 文件概览

该分支包含 UART heartbeat/printf 验证相关 STM32 工程：

```text
.gitignore
.mxproject
.project
01_LED.ioc
Appli/Core/Inc/
Appli/Core/Src/
STM32CubeIDE/
Secure_nsclib/
```

重点文件包括 `app_uart.c`、`app_uart.h`、`app_main`、`main` 和 STM32CubeIDE 工程配置。

## 是否存在未合并硬件工作

存在。`origin/main` 当前基本为空，仅有 `.gitattributes`；三个 `codex/*` 分支均包含 STM32 工程内容，且彼此覆盖范围不同。本次软件上传未合并、未覆盖、未删除任何硬件分支。

## 本次建议上传分支

建议上传到：

```text
software/cloud-edge-dashboard
```

原因：该远端分支在检查时不存在，适合作为本次 Dashboard、云端、Edge Function、测试脚本、文档和 AI 部署辅助文件的独立软件分支。
