# 基于 STM32N647 的智慧农业病虫害视觉诊断与闭环干预系统

本仓库用于汇总智慧农业项目的软件、云端、Dashboard、Supabase Edge Function、测试脚本、文档和 AI 部署辅助文件。

## 系统链路

```text
STM32N647 -> L160/L610 -> Supabase Edge Function -> Supabase PostgreSQL -> Dashboard
```

正式设备上报 URL：

```text
https://xodxtuctgknnftfgoray.supabase.co/functions/v1/device-upload
```

## 本分支包含

- `software/agri_dashboard/`：Flask Dashboard 后端、前端页面、Supabase 配置、Edge Function、测试脚本和软件文档。
- `software/agri_dashboard/supabase/functions/device-upload/`：设备上报 Edge Function。
- `software/stm32n6_ai_deploy/`：STM32N6 AI 部署辅助工具、分析报告、硬件交付材料和模型标签。
- `models/`：模型标签、模型来源说明和已检查的小型 ONNX 模型。
- `docs/`：项目概览、远端状态报告、软件云端摘要、安全说明和后续合并建议。
- `firmware/README.md`：硬件分支说明。

## 本分支不包含

- 真实密钥、数据库密码、Supabase service role key 或 access token。
- `.env` 或 `.env.*` 本地环境文件，`.env.example` 仅保留占位示例。
- 大型训练数据集、完整训练 `runs/` 输出和未筛选缓存。
- 未经确认的大模型文件。
- 硬件同学在 `codex/*` 分支中的 STM32 工程内容。

## 硬件分支说明

STM32 工程当前主要保存在远端硬件分支：

- `codex/sensor-i2c-scan`
- `codex/relay`
- `codex/uart-printf`

本软件分支不合并、不覆盖这些硬件分支。后续如需整合固件，应单独创建硬件集成分支并做代码审查。

## 运行 Dashboard

```powershell
cd software\agri_dashboard
python -m pip install -r requirements.txt
copy .env.example .env
scripts\start_dashboard_8001.bat
```

浏览器打开：

```text
http://127.0.0.1:8001
```

如果未配置 Supabase 或 YOLO 模型，后端保留 mock fallback，仍可用于演示页面流程。

## Supabase Edge Function 部署

```powershell
cd software\agri_dashboard
supabase login
supabase link --project-ref xodxtuctgknnftfgoray
supabase functions serve device-upload --no-verify-jwt --env-file .env
supabase functions deploy device-upload --no-verify-jwt
```

数据库迁移和 RLS 策略：

```text
software/agri_dashboard/supabase/migrations/001_formal_schema_extension.sql
software/agri_dashboard/supabase/migrations/002_hardware_status_fields.sql
software/agri_dashboard/supabase/rls_formal_policies.sql
```

## 设备安全约定

- JSON v1.0 body 不放 token。
- Header 使用 `X-Device-Id`、`X-Device-Token`、`X-Device-Signature`。
- `X-Device-Signature` 为 `HMAC-SHA256(raw JSON body)` 的十六进制签名。
- Service role key 只允许配置为 Supabase Edge Function secret，不进入前端、设备端示例或 Git 历史。

## 测试入口

```powershell
cd software\agri_dashboard
python -m unittest discover -s tests -p "test_*.py"
python tools\test_dashboard_e2e.py --url http://127.0.0.1:8001
python tools\test_edge_device_upload.py https://xodxtuctgknnftfgoray.supabase.co/functions/v1/device-upload
python tools\test_edge_invalid_upload.py https://xodxtuctgknnftfgoray.supabase.co/functions/v1/device-upload
```

## 后续合并计划

建议先从本软件分支向 `main` 发起 PR，审查范围限定在 `software/`、`models/`、`docs/`、`firmware/README.md` 和仓库级配置。硬件分支建议后续单独合并，避免软件上传影响 STM32 工程。
