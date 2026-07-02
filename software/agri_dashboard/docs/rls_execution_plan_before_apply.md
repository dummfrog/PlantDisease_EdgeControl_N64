# RLS 正式策略执行前计划

当前未执行 `supabase/rls_formal_policies.sql`，原因是线上 Edge Function 尚未完成成功写库验收：远端缺少 `DEVICE_TOKEN_NODE01` 等设备 secrets，正常请求被安全拒绝。

## 执行前条件

1. Supabase Edge Function `device-upload` 已部署且 `verify_jwt=false`。
2. Supabase Secrets 已设置：
   - `SUPABASE_URL`
   - `SUPABASE_SERVICE_ROLE_KEY`
   - `DEVICE_TOKEN_NODE01/02/03`
   - `DEVICE_HMAC_SECRET_NODE01/02/03`
   - `REQUIRE_HMAC=true`
3. 正常线上测试返回 `ok: true`。
4. `diagnosis_records` 出现 `inference_source=edge_function_l160_http` 的新记录。
5. Dashboard 8001 可读取并展示最新记录。
6. 异常 token、错误 HMAC、错误 disease_id、错误 confidence 均被拒绝。

## 建议执行顺序

1. 设置 Secrets。
2. 运行线上正常测试：
   ```powershell
   python tools\test_edge_device_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
   ```
3. 运行异常测试：
   ```powershell
   python tools\test_edge_invalid_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
   ```
4. 打开 Dashboard，确认最新记录可见。
5. 在 Supabase SQL Editor 中审阅并执行：
   ```text
   supabase/rls_formal_policies.sql
   ```
6. 再次运行正常测试和异常测试。
7. 再次确认 Dashboard 可读。

## 回滚思路

如果正式策略执行后 Dashboard 无法读取：

1. 暂时恢复 `anon/authenticated` 的 select 权限和 select policy。
2. 不恢复匿名 insert。
3. 继续保持设备写入只走 Edge Function。

如果 Edge Function 写入失败：

1. 不清空历史数据。
2. 不删除表。
3. 检查 Secrets、函数日志和 `diagnosis_records` 字段约束。

## 禁止事项

- 不执行清空表数据。
- 不删除历史记录。
- 不执行 `supabase stop --no-backup`。
- 不把 service role key、device token、HMAC secret 写入文档或前端。
