# 正式数据库结构与 RLS 说明

## 范围

本文件对应 Prompt 2。设备上传 JSON v1.0 body 保持原始扁平结构，不新增 token、signature、nonce、message_id 等字段。安全认证走 HTTP Header，写入由 Supabase Edge Function 使用 service role 完成。

## 执行顺序

1. 在 Supabase Dashboard 备份当前 `public.diagnosis_records` 和 `public.action_records`。
2. 在 SQL Editor 执行 `supabase/migrations/001_formal_schema_extension.sql`。
3. 检查新增表和视图是否创建成功。
4. 在确认 Edge Function 可写入后，执行 `supabase/rls_formal_policies.sql` 切换正式 RLS。
5. 使用 Dashboard 和测试脚本验证读取、写入、拒绝非法请求。

所有脚本都不包含 `truncate`、`delete` 或清空历史数据的操作。

## 兼容扩展

保留现有 `diagnosis_records` 字段，新增：

- `model_version`
- `latency_ms`
- `raw_payload`
- `system_status`
- `sensor_status`

同时扩展 `risk_level` 约束，兼容旧值 `MEDIUM_HIGH` 和正式值 `CRITICAL`。`pump_action`、`fan_action` 兼容 `NONE` 和 `REQUEST`，避免设备 JSON v1.0 的合法枚举被数据库拒绝。`REQUEST` 表示当前固件只记录动作请求，不代表水泵或风扇已经真实启动。

硬件状态字段说明：

- `liquid_level`：当前可继续上传占位 `OK`，后续液位传感器接入后改为真实值。
- `soil_moisture_percent`：当前可继续上传 mock `42`，后续土壤湿度传感器接入后改为真实值。
- `current_ma`：已支持 INA219 真实电流。
- `system_status`：用于设备在线、启动、错误等系统状态判断。
- `sensor_status`：用于传感器健康状态判断。

## 新增表

- `device_registry`：管理 Node01/Node02/Node03，记录区域、作物、状态和 token 版本，不保存明文 token。
- `device_telemetry`：保存环境传感器时序数据。
- `actuation_logs`：保存水泵、风扇、继电器动作日志。
- `security_events`：保存非法 token、签名错误、过期 timestamp、非法 disease_id 等安全事件。
- `model_versions`：保存 PC YOLO、ONNX、STM32 NPU 和设备报告模型版本。

## 索引

正式 Dashboard 查询索引包括：

- `diagnosis_records(device_id, created_at desc)`
- `diagnosis_records(risk_level, created_at desc)`
- `diagnosis_records(disease_class_id, created_at desc)`
- `device_telemetry(device_id, created_at desc)`
- `actuation_logs(device_id, created_at desc)`
- `security_events(device_id, created_at desc)`

## Dashboard 视图

- `dashboard_recent_records_v`
- `dashboard_node_status_v`
- `dashboard_risk_summary_v`
- `dashboard_heatmap_24h_v`
- `dashboard_alerts_v`

视图使用 `security_invoker = true`，使 Postgres 15+ 下视图调用遵循底层表 RLS。Supabase 文档要求 public schema 表开启 RLS，并显式授予角色所需权限；2026 年 Supabase 变更也强调新表不会总是默认暴露到 Data API，因此迁移和 RLS 文件都写明了显式 `GRANT`。

## 正式 RLS 策略

正式策略要点：

- `diagnosis_records` 开启 RLS。
- `anon` / `authenticated` 不允许直接 insert。
- 设备不直接持有 Supabase anon key 或 service role key。
- L160/L610 只 POST 到 Edge Function。
- Edge Function 校验 Header token/HMAC/timestamp 后，用 service role 写入。
- Dashboard 只读。演示阶段可以通过 Flask 后端或必要视图读取。
- `security_events` 不对匿名前端开放。
- `device_registry` 只开放设备展示所需字段，不开放密钥或明文 token。

## 对当前 Dashboard 的影响

不会破坏当前 Dashboard：

- 旧表 `diagnosis_records` 保留。
- 旧字段保留。
- `/api/history`、`/api/stats` 仍可读原表。
- 正式视图是新增能力，后端可逐步切换。
- Edge Function 使用 service role，RLS 不会阻止服务端写入。

正式切换后，匿名 insert 被禁止；这是预期行为。
