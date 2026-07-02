# Software Cloud Edge Summary

## Dashboard

`software/agri_dashboard` 包含 Flask 后端、原生前端页面、Supabase 数据访问、测试脚本和部署文档。默认本地端口为 `8001`。

## Edge Function

设备上报入口位于：

```text
software/agri_dashboard/supabase/functions/device-upload/index.ts
```

线上 URL：

```text
https://xodxtuctgknnftfgoray.supabase.co/functions/v1/device-upload
```

## 数据库

数据库建表、迁移和 RLS 文件位于：

```text
software/agri_dashboard/supabase_schema.sql
software/agri_dashboard/supabase/migrations/
software/agri_dashboard/supabase/rls_formal_policies.sql
```

## 测试脚本

测试入口位于：

```text
software/agri_dashboard/tests/
software/agri_dashboard/tools/
software/agri_dashboard/scripts/
```
