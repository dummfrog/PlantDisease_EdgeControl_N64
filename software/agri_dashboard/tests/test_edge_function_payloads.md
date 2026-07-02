# Edge Function Payload Test Cases

设备 JSON v1.0 body 保持不变，认证字段只放在 HTTP Header。

## 正常用例

- Node01 高风险番茄晚疫病，`disease_id = 6`，`confidence = 0.92`。
- `pump_action = REQUEST`、`fan_action = REQUEST` 表示只记录动作请求，不代表真实启动。
- `system_status = ONLINE`、`sensor_status = OK` 可用于设备在线和传感器健康判断。
- Node03 健康作物，`disease_id = 7`，`risk_level = LOW`。
- Header 包含 `X-Device-Id`、`X-Device-Token`、`X-Timestamp`、`X-Signature`。

## 异常用例

- 缺少 `X-Device-Token`。
- `X-Device-Token` 错误。
- Header `X-Device-Id` 与 body `device_id` 不一致。
- `X-Signature` 错误。
- `X-Timestamp` 过期。
- `schema_version = 2.0`。
- `disease_id = -1` 或 `disease_id = 99`。
- `confidence = -0.1` 或 `confidence = 1.5`。
- 非 POST。
- 非 `Content-Type: application/json`。

## 安全规则用例

- `liquid_level = LOW` 时，Edge Function 应将 `pump_action` 归一化为 `OFF`。
- `confidence < 0.6` 时，建议重新采集或人工复核，禁止自动喷药。
- `alarm = CURRENT_ABNORMAL` 或电流异常时，应禁止执行机构动作，并提示检查泵、风扇、继电器和供电回路。
