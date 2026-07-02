# 软件 Release Checklist

## 代码检查

- [ ] Git 工作区只包含本次交付相关变更。
- [ ] 未提交 `.env`、真实 token、service role key、HMAC secret。
- [ ] 设备 JSON v1.0 body 未新增鉴权字段。

## 编译与测试

- [ ] `python -m py_compile backend\app.py backend\database.py backend\expert_rules.py backend\validators.py`
- [ ] `python -m unittest discover -s tests -p "test_*.py"`
- [ ] `/api/health` 正常。
- [ ] `/api/stats` 正常。
- [ ] `/api/history` 正常。
- [ ] `/api/upload` 图片上传仍可用。
- [ ] `/api/simulate_device` 正常。

## Edge Function

- [ ] TypeScript/Deno 语法检查通过。
- [ ] 本地 serve 可运行。
- [ ] 正常 JSON v1.0 上传成功。
- [ ] 错误 token 被拒绝。
- [ ] 错误 HMAC 被拒绝。
- [ ] Header/body device_id 不一致被拒绝。
- [ ] 非法 disease_id 被拒绝。
- [ ] `liquid_level = LOW` 禁止喷药。
- [ ] `confidence < 0.6` 触发人工复核。

## Supabase

- [ ] Secrets 已设置且未写入文档明文。
- [ ] 正式迁移已执行。
- [ ] RLS 正式策略已执行。
- [ ] 匿名 insert 被禁止。
- [ ] Edge Function service role 仍可写入。
- [ ] Dashboard 仍可读取。

## Dashboard

- [ ] 最近记录刷新正常。
- [ ] 设备在线状态正常。
- [ ] 告警中心正常。
- [ ] 热力图正常。
- [ ] 筛选正常。
- [ ] CSV 导出不包含 token 或密钥。
- [ ] 大屏投屏布局可读。
- [ ] E2E 截图归档。

## 文档与演示

- [ ] 部署文档完整。
- [ ] Secrets 文档完整。
- [ ] L160/L610 联调文档完整。
- [ ] 测试计划和报告模板完整。
- [ ] 答辩 Q&A 覆盖关键问题。
- [ ] 风险与备用方案明确只是预案，不是正式主线。
