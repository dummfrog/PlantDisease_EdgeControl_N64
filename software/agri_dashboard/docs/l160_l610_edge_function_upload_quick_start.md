# L160/L610 连接 Supabase Edge Function 快速联调

## 角色说明

L160/L610 的角色是 4G HTTP 客户端，不是服务器。

正式上传目标是 Supabase Edge Function：

```text
https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

本地 Flask Dashboard 只用于页面展示和 PC 端 YOLO 图片上传演示，不作为 L160/L610 正式公网入口。

## HTTP 参数

```text
Method: POST
Header: Content-Type: application/json
Header: X-Device-Id: Node01
Header: X-Device-Token: <当前节点 token>
Header: X-Signature: <HMAC-SHA256(body, 当前节点 HMAC secret) 十六进制>
URL: https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

## 设备 JSON v1.0 示例

认证信息通过 HTTP Header 发送，不要把真实 token 或 HMAC secret 写入公开文档、前端或 Dashboard。JSON body 保持 v1.0 业务数据结构，不新增认证字段。

```json
{
  "schema_version": "1.0",
  "device_id": "Node01",
  "timestamp": "2026-06-26T12:00:00+08:00",
  "uptime_ms": 123456,
  "disease_id": 6,
  "disease": "Tomato_Late_Blight",
  "disease_cn": "番茄晚疫病",
  "confidence": 0.92,
  "risk_level": "HIGH",
  "temperature_c": 28.6,
  "humidity_percent": 78.2,
  "light_lux": 13500,
  "soil_moisture_percent": 42,
  "liquid_level": "OK",
  "pump_action": "REQUEST",
  "fan_action": "REQUEST",
  "pump_duration_s": 5,
  "fan_duration_s": 10,
  "current_ma": 680,
  "system_status": "ONLINE",
  "sensor_status": "OK",
  "alarm": "NONE"
}
```

## 成功返回

```json
{
  "ok": true,
  "id": 123,
  "message": "device record inserted by edge function"
}
```

## 失败返回

```json
{
  "ok": false,
  "error": "具体错误原因"
}
```

常见失败：

- `missing X-Device-Token`
- `invalid X-Device-Token`
- `invalid X-Signature`
- `disease_id must be an integer from 0 to 7`
- `Content-Type must be application/json`
- `method not allowed; use POST`

## STM32 拼接 JSON 注意事项

- JSON 必须是 UTF-8 文本。
- `confidence` 必须是 0 到 1 的数字，不要传百分比字符串。
- `disease_id` 必须是整数 0 到 7。
- `timestamp` 建议使用 ISO 8601，例如 `2026-06-26T12:00:00+08:00`。
- `pump_action` / `fan_action` 可传 `ON`、`OFF`、`NONE`、`REQUEST`。
- 当前硬件阶段如只记录请求、不真实启动水泵/风扇，建议上传 `REQUEST`。
- `liquid_level` 仍可上传占位 `OK`。
- `soil_moisture_percent` 仍可上传 mock `42`。
- `current_ma` 用 INA219 真实电流。
- `system_status` 用于设备在线/异常判断。
- `sensor_status` 用于传感器健康判断。
- 液位为 `LOW` 时，Edge Function 会禁止喷药并写入报警建议。
- 低置信度 `< 0.6` 时，Edge Function 会禁止自动喷药并建议人工复核或重新采集。
- `device_token` / HMAC secret 只放在对应设备端，不出现在 Dashboard 和前端。

## AT 指令流程占位

具体 AT 指令以 L160/L610 固件手册为准，联调顺序建议如下：

1. 模组上电。
2. 检查串口通信。
3. 检查 SIM 卡状态。
4. 注册 4G 网络。
5. 激活 PDP。
6. 设置 HTTP URL。
7. 设置 `Content-Type: application/json`。
8. 发送 POST Body。
9. 读取 HTTP 状态码。
10. 读取响应 JSON。
11. STM32 根据 `ok` 字段和 `error` 字段记录上报结果。

## 硬件验收 checklist

- 串口能和 L160/L610 通信。
- 能注册 4G 网络。
- 能 POST 到 Supabase Edge Function。
- Edge Function 返回 `ok: true`。
- Supabase `diagnosis_records` 新增记录。
- Dashboard 最近诊断记录刷新。
- 异常 token 被拒绝。
- 异常 disease_id 被拒绝。
- 低液位时不喷药。
- 低置信度时不自动喷药。
- `device_token` 没有写入 Supabase 表。
# 正式版补充：L160/L610 Edge Function 上传快速开始

L160/L610 是 4G HTTP 客户端，不是服务器。它不能访问 `127.0.0.1`，不能访问 `localhost`，普通内网 IP 也不适合真实 4G 公网上报。Dashboard 可以本机投屏，但正式设备上传入口必须是公网 HTTPS。

## 正式上传目标

```text
Method: POST
URL: https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
Content-Type: application/json
X-Device-Id: Node01
X-Device-Token: 对应节点token
X-Timestamp: 当前 ISO 8601 时间
X-Signature: HMAC_SHA256(raw_body, device_hmac_secret)
```

调试阶段如果硬件还未实现 HMAC，可以临时将 Edge Function Secret `REQUIRE_HMAC=false`，但正式版建议必须为 `true`。不要把真实 token、HMAC secret、service role key 写进文档或串口日志。硬件端只保存本节点自己的 token 和 HMAC secret。

## JSON v1.0 示例

```json
{
  "schema_version": "1.0",
  "device_id": "Node01",
  "timestamp": "2026-06-23T20:30:00+08:00",
  "uptime_ms": 123456,
  "disease_id": 6,
  "disease": "Tomato_Late_Blight",
  "disease_cn": "番茄晚疫病",
  "confidence": 0.92,
  "risk_level": "HIGH",
  "temperature_c": 28.6,
  "humidity_percent": 78.2,
  "light_lux": 13500,
  "soil_moisture_percent": 42,
  "liquid_level": "OK",
  "pump_action": "REQUEST",
  "fan_action": "REQUEST",
  "pump_duration_s": 5,
  "fan_duration_s": 10,
  "current_ma": 680,
  "system_status": "ONLINE",
  "sensor_status": "OK",
  "alarm": "NONE"
}
```

不要在 body 中新增 `device_token`、`signature`、`nonce` 或 `message_id`。

## STM32 拼接注意事项

- 字符串必须是合法 JSON。
- 字符串字段必须带双引号。
- 数值字段不要带中文单位。
- `confidence` 是 0 到 1 的小数。
- `disease_id` 是 0 到 7 的整数。
- `timestamp` 使用 ISO 8601 格式。
- body 中 `device_id` 必须和 Header `X-Device-Id` 一致。
- POST 前先打印完整 JSON 到串口，便于定位格式问题。

## AT 指令流程

1. 模组上电。
2. 检查 AT 通信。
3. 检查 SIM 卡。
4. 检查信号强度。
5. 检查网络注册。
6. 激活 PDP。
7. 初始化 HTTP。
8. 设置 URL。
9. 设置 HTTPS。
10. 设置 Header。
11. 写入 POST Body。
12. 发起 POST。
13. 读取 HTTP 状态码。
14. 读取服务器响应。
15. 关闭 HTTP。
16. 异常重试。

## 成功返回

```json
{
  "ok": true,
  "id": 123,
  "message": "device record inserted by edge function"
}
```

## 常见失败

- token 错误。
- 签名错误。
- device_id 不一致。
- disease_id 越界。
- confidence 越界。
- JSON 格式错误。
- Supabase 写入失败。

## 硬件联调 Checklist

- [ ] STM32 串口能输出 heartbeat。
- [ ] STM32 能和 L160/L610 AT 通信。
- [ ] SIM 卡识别正常。
- [ ] 4G 网络注册成功。
- [ ] PDP 激活成功。
- [ ] 能访问 Edge Function URL。
- [ ] 能 POST JSON。
- [ ] Edge Function 返回 ok。
- [ ] Supabase 新增记录。
- [ ] Dashboard 最近诊断记录刷新。
- [ ] 错误 token 被拒绝。
- [ ] 错误 disease_id 被拒绝。
- [ ] 液位 LOW 时禁止喷药。
- [ ] 低置信度时提示重新采集。
- [ ] 网络断开后能重试。
- [ ] 串口日志能定位失败原因。

## 答辩解释

不用 localhost，是因为 L160/L610 位于 4G 公网侧，`127.0.0.1` 指向模组自身而不是电脑。使用 Edge Function，是为了让公网设备先经过服务端鉴权和 JSON 校验，再写入 Supabase。token/HMAC 放 Header，是为了保持 JSON v1.0 body 原协议不变。设备不直连 Supabase REST，是为了避免泄露数据库 key 和绕过正式 RLS 边界。
