# Edge Function 正式验收 Checklist

## 环境

- [x] Node.js 20+ 可用。
- [x] Supabase CLI 已安装为项目 dev dependency。
- [x] Deno 已安装，可通过 winget 安装路径运行。
- [x] Docker Desktop 已安装。
- [x] Docker daemon 可用。

## 配置

- [x] `.env.example` 已补齐，无真实密钥。
- [x] `.env` 已备份。
- [x] `.env` 已追加本地测试 device token / HMAC secret。
- [ ] `.env` 含真实 `SUPABASE_SERVICE_ROLE_KEY`。
- [ ] 线上 Supabase Secrets 已全部设置。

## Edge Function

- [x] `device-upload` 已部署到 Supabase。
- [x] `verify_jwt=false`。
- [x] 正式 URL：`https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload`。
- [x] Header token 鉴权已实现。
- [x] HMAC-SHA256 鉴权已实现。
- [x] JSON body v1.0 未新增认证字段。

## 测试

- [x] `node --check supabase\functions\device-upload\index.ts` 通过。
- [x] `deno check --minimum-dependency-age 0 supabase/functions/device-upload/index.ts` 通过。
- [x] Python 编译检查通过。
- [x] 线上异常请求全部被拒绝。
- [ ] 本地 `supabase functions serve` 成功。
- [ ] 本地正常请求成功写库。
- [ ] 线上正常请求成功写库。
- [ ] Dashboard 显示 Edge Function 写入数据。

## 安全

- [x] 未把真实 service role key 写入代码或文档。
- [x] 未把真实 device token 写入代码或文档。
- [x] `diagnosis_records` 无 token 字段。
- [x] 异常 token 被拒绝。
- [x] 错误 HMAC 被拒绝。
- [x] 错误 disease_id 被拒绝。
- [x] 错误 confidence 被拒绝。

## 阻塞项

- CLI 非 TTY 登录失败，需要 `SUPABASE_ACCESS_TOKEN` 或用户手动 `npx supabase login --token <token>`。
- 线上设备 token/HMAC secrets 尚未设置。
- `.env` 缺真实 `SUPABASE_SERVICE_ROLE_KEY`。
- 本地 Supabase stack 启动未完成，`supabase start` 卡住未创建容器。
