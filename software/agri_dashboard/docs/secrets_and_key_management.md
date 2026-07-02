# Secrets 与密钥管理

## 密钥清单

- Supabase publishable/anon key：可用于只读 Dashboard 场景，必须配合 RLS 和最小权限。
- Supabase service role key：只允许在 Edge Function 或可信服务端使用。
- `DEVICE_TOKEN_NODE01/02/03`：设备身份 token，放 Supabase Secrets。
- `DEVICE_HMAC_SECRET_NODE01/02/03`：设备 HMAC secret，放 Supabase Secrets。
- 硬件端本地 token/HMAC secret：只保存本节点自己的值。

## 可以放 `.env` 的内容

- 本地 Dashboard 的 `SUPABASE_URL`。
- 本地开发使用的 publishable/anon key。
- 本地测试 Edge Function 用的测试 token。
- `YOLO_MODEL_PATH`、`FLASK_HOST`、`FLASK_PORT` 等本机配置。

`.env` 必须被 `.gitignore` 忽略，不提交。

## 只能放 Supabase Secrets 的内容

- 线上 `SUPABASE_SERVICE_ROLE_KEY`。
- 线上设备 token。
- 线上 HMAC secret。
- `REQUIRE_HMAC`。

## 不能进入前端或硬件文档

- service role key。
- 任意真实设备 token。
- 任意真实 HMAC secret。
- Supabase 数据库连接字符串。

## 轮换策略

设备 token 轮换：

1. 在 Supabase Secrets 设置新 token。
2. 更新 `device_registry.token_version`。
3. 下发到对应设备。
4. 现场验证新 token 上传成功。
5. 移除旧 token。

HMAC secret 轮换同理。轮换期间可短时维护双版本策略，但正式文档中不记录真实值。

## 泄露处理

service role key 泄露：

1. 立即在 Supabase Dashboard 轮换 key。
2. 更新 Edge Function Secret。
3. 重新部署或重启函数环境。
4. 检查 `security_events`、数据库写入日志和异常访问。
5. 复盘泄露路径，移除所有明文记录。
