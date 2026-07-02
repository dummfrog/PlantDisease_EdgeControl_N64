# 正式测试计划

## 测试范围

- JSON v1.0 校验。
- 专家规则归一化。
- Supabase Edge Function 正常与异常上传。
- Flask Dashboard API。
- Dashboard 浏览器 E2E。
- Edge Function 简单压力测试。

设备上传 JSON v1.0 body 不改，所有鉴权字段通过 Header 发送。

## 测试环境

- 本地 Dashboard：`http://127.0.0.1:8001`
- 本地 Edge Function：`http://127.0.0.1:54321/functions/v1/device-upload`
- 线上 Edge Function：`https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload`

## 测试命令

```powershell
python -m unittest discover -s tests -p "test_*.py"
python tools\test_edge_device_upload.py http://127.0.0.1:54321/functions/v1/device-upload
python tools\test_edge_invalid_upload.py http://127.0.0.1:54321/functions/v1/device-upload
python tools\load_test_edge_upload.py http://127.0.0.1:54321/functions/v1/device-upload --count 100
python tools\test_dashboard_e2e.py --url http://127.0.0.1:8001
```

## 正常用例

- Node01 高风险数据上传成功。
- 健康类默认 LOW 且关闭泵/风扇。
- `/api/health`、`/api/stats`、`/api/history` 正常返回。
- 图片上传后写入诊断记录。
- Dashboard 图表和历史记录渲染。

## 异常与安全用例

- 缺少 token 被拒绝。
- 错误 token 被拒绝。
- Header 与 body device_id 不一致被拒绝。
- 错误 HMAC 被拒绝。
- 过期 timestamp 被拒绝。
- 非法 disease_id、confidence、risk_level 被拒绝。
- `liquid_level = LOW` 禁止喷药。
- 低置信度要求人工复核。
- 电流异常禁止执行机构动作。

## 验收标准

- Python 编译检查通过。
- 单元测试通过。
- Edge Function 正常请求成功，异常请求被拒绝。
- Dashboard E2E 无 console error、无网络失败。
- 压力测试不清空数据库，只统计成功率和平均耗时。
