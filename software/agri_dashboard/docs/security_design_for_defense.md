# 答辩安全设计

## 设备身份校验

设备请求必须包含 `X-Device-Id`，并与 body 中 `device_id` 一致。

## Header token

每个节点使用独立 token，通过 `X-Device-Token` 发送。token 只保存在设备和 Supabase Secrets，不写入 JSON body。

## HMAC 签名

`X-Signature = HMAC_SHA256(raw_body, device_hmac_secret)`。签名对象是原始 body 字符串，不是重新 stringify 后的 JSON。

## 时间戳

`X-Timestamp` 用于防止过期请求。Edge Function 校验时间窗口。

## JSON v1.0 校验

Edge Function 校验 schema、device_id、disease_id、confidence、枚举值和合理传感器范围。

## RLS 策略

匿名用户不能直接 insert。设备写入只通过 Edge Function service role。Dashboard 只读必要表或视图。

## 密钥边界

service role key 只在 Edge Function。前端不保存高权限密钥，硬件不保存 Supabase service role key。

## 安全事件

非法 token、签名错误、非法 disease_id、过期 timestamp 可记录到 `security_events`，用于审计和答辩展示。

## 为什么不让 L160 直连 Supabase REST

直连需要让设备持有 Supabase key，泄露后会扩大数据库攻击面。Edge Function 能先完成鉴权、校验和归一化，再由服务端安全写入。
