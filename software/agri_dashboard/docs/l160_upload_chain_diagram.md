# L160/L610 上报链路图

```text
STM32N647
  生成 JSON v1.0
  设置 Header: X-Device-Id / X-Device-Token / X-Timestamp / X-Signature
        ↓ UART
L160/L610 4G HTTP 客户端
        ↓ HTTPS POST
Supabase Edge Function device-upload
  设备身份校验
  HMAC 校验
  JSON v1.0 校验
  专家规则归一化
        ↓ service role insert
Supabase PostgreSQL diagnosis_records
        ↓ read only
Flask Dashboard 8001
```

L160/L610 不能访问 `127.0.0.1` 或 `localhost`。上传目标必须是公网 Edge Function。JSON body 保持原协议，认证字段只在 Header。
