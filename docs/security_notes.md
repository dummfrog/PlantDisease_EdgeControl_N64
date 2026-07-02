# Security Notes

本分支上传前按以下原则处理敏感信息：

- 不上传 `.env` 或 `.env.*`。
- `.env.example` 只允许保存占位值。
- 不上传 Supabase service role key、access token、真实设备 token、真实 HMAC secret 或数据库密码。
- 不上传 `node_modules/`、虚拟环境、缓存、日志目录和完整训练输出。
- 设备认证信息放在 HTTP Header，JSON v1.0 body 不放 token。
- Edge Function 使用 `HMAC-SHA256(raw JSON body)` 校验 `X-Device-Signature`。

如后续发现真实密钥进入 Git 历史，应立即撤销密钥、轮换 Supabase secrets，并避免通过 force push 处理公共协作分支。
