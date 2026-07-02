# 正式安全模型

## 演示策略与正式策略

演示阶段为了快速验证 Dashboard，`anon/authenticated` 可能被允许直接 insert `diagnosis_records`。正式阶段撤销这一能力，设备写入统一经过 Supabase Edge Function。

正式主线：

```text
STM32N647
-> L160/L610 4G HTTP POST
-> Supabase Edge Function device-upload
-> Header 鉴权 / HMAC / timestamp / JSON v1.0 校验 / 专家规则归一化
-> Supabase PostgreSQL
-> Dashboard 只读展示
```

Cloudflare Tunnel / ngrok 只能作为应急联调备用方案，不是正式主线。

## 为什么设备不能持有 Supabase key

L160/L610 是现场设备，固件和串口日志都有泄露风险。让设备持有 Supabase anon key 会把数据库 Data API 暴露给设备侧；让设备持有 service role key 更危险，因为 service role 可绕过 RLS。

正式方案中，设备只保存本节点自己的 token 和 HMAC secret。即使单个设备泄露，也只影响该节点，可通过 Supabase Secrets 轮换。

## 为什么 Edge Function 使用 service role

Edge Function 是服务端受控环境，可读取 Supabase Secrets。它先校验：

- `X-Device-Id`
- `X-Device-Token`
- `X-Timestamp`
- `X-Signature`
- 原始 JSON v1.0 body

校验通过后才使用 service role 写入数据库。service role 不进入前端、README、硬件示例或串口日志。

## Dashboard 为什么只读

Dashboard 面向投屏展示和本地演示，不应直接成为设备写入口。它读取统计、历史、告警和热力图，最多通过本地 Flask 后端做 PC 图片上传 YOLO 演示。

正式设备入口只有 Edge Function。

## RLS 如何保护数据

RLS 开启后：

- 匿名角色没有 insert 权限。
- `security_events` 不开放给匿名前端。
- `device_registry` 只开放展示字段。
- 视图使用 `security_invoker = true`，避免视图默认绕过底层表策略。
- service role 仅用于 Edge Function 和可信服务端。

## 执行前备份

在 Supabase SQL Editor 执行正式 SQL 前，先导出：

- `diagnosis_records`
- `action_records`

迁移脚本不会清空历史数据，但正式切换 RLS 会改变匿名写入权限，应先确认 Edge Function 和 Dashboard 读取链路已经可用。
