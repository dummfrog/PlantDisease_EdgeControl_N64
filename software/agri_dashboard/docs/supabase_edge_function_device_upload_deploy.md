# Supabase Edge Function 设备上报部署说明

## 方案定位

```text
STM32N647 + L160/L610
↓
Supabase Edge Function: device-upload
↓
Supabase PostgreSQL: diagnosis_records
↓
Dashboard
```

正式设备上报入口是 Supabase Edge Function，不是本地 Flask，不是 Cloudflare Tunnel，也不是 ngrok。

## 为什么正式版使用 Supabase Edge Function

- 设备需要公网 HTTPS 入口。
- 不暴露 Supabase service role key 到硬件。
- 不依赖本地电脑开机。
- 比 ngrok / tunnel 更适合正式交付和答辩演示。
- Edge Function 可以集中完成设备 token 校验、协议校验、风险归一化和安全策略。

## 初始化和链接项目

```bash
supabase login
supabase link --project-ref xodxtuctgknnftfgoray
```

## 设置 Secrets

不要把真实 service role key 或真实设备 token 写入文档、前端、README 或硬件示例仓库。

```bash
supabase secrets set SUPABASE_URL="https://xodxtuctgknnftfgoray.supabase.co"
supabase secrets set SUPABASE_SERVICE_ROLE_KEY="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE01="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE02="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE03="不要写真实值到文档"
```

本地测试时可以在 `.env` 中临时放 `DEVICE_TOKEN_NODE01` 等测试 token；不要提交真实 token。

## 本地运行 Edge Function

```bash
supabase functions serve device-upload --no-verify-jwt --env-file .env
```

本地测试 URL：

```text
http://127.0.0.1:54321/functions/v1/device-upload
```

## 部署 Edge Function

```bash
supabase functions deploy device-upload --no-verify-jwt
```

使用 `--no-verify-jwt` 的原因：L160/L610 不携带 Supabase JWT，函数内部使用 Header token 和 HMAC 做设备鉴权，并使用 Edge Function Secret 中的 service role key 写入数据库。

## 线上测试

```bash
python tools/test_edge_device_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

异常 token 测试：

```bash
python tools/test_edge_invalid_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

异常 disease_id 测试：

```bash
python tools/test_edge_device_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload --bad-disease-id
```

## L160/L610 最终上传 URL

```text
https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload

如果 Supabase Dashboard 的 Functions 页面显示不同调用地址，以 Dashboard 显示为准。
```

## 数据库与 RLS

正式 RLS 策略文件：

```text
supabase/rls_formal_policies.sql
```

切换正式策略后，设备不再通过 anon key 直接 insert。正式写入路径是：

```text
设备 Header token/HMAC -> Edge Function 校验 -> service role 写入 diagnosis_records
```

## 注意事项

- `127.0.0.1` 只能本机访问。
- L160/L610 不能访问 localhost。
- L160/L610 必须访问公网 HTTPS URL。
- Dashboard 可以本机投屏，但设备上报入口必须公网。
- `SUPABASE_SERVICE_ROLE_KEY` 只允许存在于 Supabase Secrets / Edge Function 环境变量。
- 硬件端只保存对应节点的 `device_token` 和 HMAC secret。
- Edge Function 不把认证字段写入 `diagnosis_records`。
# 正式版补充：部署、Secrets 与验证

设备上传 JSON v1.0 body 保持不变；设备认证字段通过 Header 发送：

```text
X-Device-Id
X-Device-Token
X-Timestamp
X-Signature
```

正式设备入口：

```text
https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

## CLI 流程

```powershell
supabase login
supabase link --project-ref xodxtuctgknnftfgoray
```

## 设置 Secrets

不要把真实值写进文档、README、前端或硬件示例。

```powershell
supabase secrets set SUPABASE_URL="https://xodxtuctgknnftfgoray.supabase.co"
supabase secrets set SUPABASE_SERVICE_ROLE_KEY="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE01="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE02="不要写真实值到文档"
supabase secrets set DEVICE_TOKEN_NODE03="不要写真实值到文档"
supabase secrets set DEVICE_HMAC_SECRET_NODE01="不要写真实值到文档"
supabase secrets set DEVICE_HMAC_SECRET_NODE02="不要写真实值到文档"
supabase secrets set DEVICE_HMAC_SECRET_NODE03="不要写真实值到文档"
supabase secrets set REQUIRE_HMAC="true"
```

## 本地运行

```powershell
supabase functions serve device-upload --no-verify-jwt --env-file .env
python tools\test_edge_device_upload.py http://127.0.0.1:54321/functions/v1/device-upload
python tools\test_edge_invalid_upload.py http://127.0.0.1:54321/functions/v1/device-upload
```

## 部署

```powershell
supabase functions deploy device-upload --no-verify-jwt
python tools\test_edge_device_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

部署后确认：

- Supabase 新增 `diagnosis_records`。
- Dashboard 最近记录刷新。
- 错误 token、错误 HMAC、错误 disease_id 均被拒绝。
