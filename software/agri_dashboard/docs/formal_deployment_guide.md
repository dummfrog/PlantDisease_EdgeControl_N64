# 正式部署指南

## 正式软件架构

```text
STM32N647 + OV5640/NPU/传感器/继电器
-> L160/L610 4G HTTP POST
-> Supabase Edge Function device-upload
-> Supabase PostgreSQL
-> 本地 Flask Dashboard 8001 读取展示
```

设备上传 JSON v1.0 body 保持原始扁平结构。设备 token、HMAC 签名、时间戳全部通过 Header 发送。

## 本地 Dashboard 8001

```powershell
cd C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard
python -m pip install -r requirements.txt
scripts\start_dashboard_8001.bat
```

访问：

```text
http://127.0.0.1:8001
```

该服务用于本地投屏、PC 图片上传 YOLO 演示、读取 Supabase 展示，不作为 L160/L610 正式公网入口。

## Supabase Edge Function

正式设备入口：

```text
https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

部署步骤见 `docs/supabase_edge_function_device_upload_deploy.md`。

## Supabase Secrets

Secrets 只配置在 Supabase Edge Function 环境：

- `SUPABASE_URL`
- `SUPABASE_SERVICE_ROLE_KEY`
- `DEVICE_TOKEN_NODE01`
- `DEVICE_TOKEN_NODE02`
- `DEVICE_TOKEN_NODE03`
- `DEVICE_HMAC_SECRET_NODE01`
- `DEVICE_HMAC_SECRET_NODE02`
- `DEVICE_HMAC_SECRET_NODE03`
- `REQUIRE_HMAC`

不要把真实值写入 README、前端、硬件示例或串口日志。

## RLS 正式策略

先执行 `supabase/migrations/001_formal_schema_extension.sql`，再执行 `supabase/rls_formal_policies.sql`。

正式策略会撤销匿名 insert。Edge Function 使用 service role 写入，Dashboard 只读。

## L160/L610 公网 URL

L160/L610 是 4G HTTP 客户端，必须访问公网 HTTPS URL，不能访问 `127.0.0.1` 或 `localhost`。

## 应急备选

Cloudflare Tunnel / ngrok 仅作为现场应急联调备用方案。正式主线始终是 Supabase Edge Function。
