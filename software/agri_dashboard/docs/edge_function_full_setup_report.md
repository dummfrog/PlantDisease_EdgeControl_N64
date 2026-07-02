# Edge Function Full Setup Report

## 时间

2026-06-26 12:29 - 13:00 Asia/Shanghai

## 安装结果

- Node.js：已存在，版本 `v24.13.0`。
- npx：已存在，版本 `11.6.2`。
- Supabase CLI：已通过 `npm install supabase --save-dev` 安装到项目本地，版本 `2.108.0`。
- Deno：已通过 winget 安装，版本 `2.9.0`。当前 shell PATH 未刷新，使用 winget 安装路径执行。
- Docker Desktop：已通过 winget 安装，Docker CLI 版本 `29.5.3`，daemon 已启动。

## 配置结果

- `.env.example` 已新增，未包含真实密钥。
- `.env` 已备份为 `.env.backup_20260626_122932`。
- `.env` 已追加本地测试 `DEVICE_TOKEN_NODE01/02/03` 和 `DEVICE_HMAC_SECRET_NODE01/02/03`。
- `.env` 已追加 `REQUIRE_HMAC=true`。
- `.env` 仍缺真实 `SUPABASE_SERVICE_ROLE_KEY`，未伪造。
- `.gitignore` 已新增，保护 `.env` / `.env.*`，保留 `.env.example`。

## Edge Function

- 源码：`supabase/functions/device-upload/index.ts`
- 认证：`X-Device-Id`、`X-Device-Token`、`X-Timestamp`、`X-Signature`
- HMAC：`HMAC-SHA256(raw JSON body, DEVICE_HMAC_SECRET_NODExx)`，十六进制。
- JSON v1.0 body 未新增认证字段。
- 部署：已通过 Supabase MCP 部署。
- 远端状态：`ACTIVE`
- `verify_jwt=false`
- 正式 URL：`https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload`

## 检查结果

- `node --check supabase\functions\device-upload\index.ts`：通过。
- `deno check --minimum-dependency-age 0 supabase/functions/device-upload/index.ts`：通过。
- `python -m py_compile backend\app.py backend\database.py tools\test_edge_device_upload.py tools\test_edge_invalid_upload.py`：通过。
- `diagnosis_records` token 字段检查：无 token 字段。
- 代码/文档密钥扫描：未发现真实 service role key 或真实 device token。

## 本地 serve

- Docker daemon 已就绪。
- `npx supabase functions serve device-upload --no-verify-jwt --env-file .env` 初次失败：Docker CLI 不在子进程 PATH。
- 显式加入 Docker PATH 后失败：`supabase start is not running`。
- 执行 `npx supabase start` 后超时，未创建容器，进程卡住后已停止本次 CLI 进程。
- 本地 serve 未完成。

## 线上测试

- 正常请求：返回 `401`，错误为远端缺少 `DEVICE_TOKEN_NODE01` secret；未写库。
- 异常测试：全部被拒绝。
  - 缺 `X-Device-Token`
  - 错误 `X-Device-Token`
  - Header `X-Device-Id` 与 body 不一致
  - 错误 HMAC
  - 过期 timestamp
  - 错误 `schema_version`
  - `disease_id=99`
  - `confidence=1.5`
  - 错误 Content-Type

## Dashboard

- `http://127.0.0.1:8001/api/health`：正常。
- `http://127.0.0.1:8001/api/stats`：正常。
- `http://127.0.0.1:8001/api/history?limit=1`：正常。
- Playwright/Chrome 页面验证：Console errors 为空，Network failures 为空。
- 截图：`dashboard_edge_function_final_verify.png`
- 因线上正常写库被 secrets 阻塞，Dashboard 尚未显示 Edge Function 写入的新记录。

## RLS

- 未执行 `supabase/rls_formal_policies.sql`。
- 已生成 `docs/rls_execution_plan_before_apply.md`。
- 原因：Edge Function 尚未完成成功写库验收，不应贸然切换 RLS。

## 剩余阻塞

1. 需要 `SUPABASE_ACCESS_TOKEN` 或用户手动执行：
   ```powershell
   npx supabase login --token <你的 Supabase access token>
   ```
2. 需要把真实 `SUPABASE_SERVICE_ROLE_KEY` 放入 `.env` 或直接设置到 Supabase Secrets。
3. 需要设置线上 Secrets：
   ```powershell
   npx supabase secrets set --env-file .env --project-ref xodxtuctgknnftfgoray
   ```
4. 本地 Supabase stack 需要继续排查 `supabase start` 卡住原因。
