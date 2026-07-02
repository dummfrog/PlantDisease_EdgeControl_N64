# 最终交付索引

## 后端代码

- `backend/app.py`
- `backend/database.py`
- `backend/expert_rules.py`
- `backend/validators.py`

## 前端代码

- `frontend/index.html`

## Edge Function

- `supabase/functions/device-upload/index.ts`

## Supabase SQL

- `supabase_schema.sql`
- `supabase/migrations/001_formal_schema_extension.sql`
- `supabase/rls_formal_policies.sql`

## 测试脚本

- `tests/test_expert_rules.py`
- `tests/test_json_v1_validation.py`
- `tests/test_dashboard_api.py`
- `tools/test_edge_device_upload.py`
- `tools/test_edge_invalid_upload.py`
- `tools/load_test_edge_upload.py`
- `tools/test_dashboard_e2e.py`
- `tools/model_field_test_logger.py`

## 部署文档

- `docs/formal_deployment_guide.md`
- `docs/supabase_edge_function_device_upload_deploy.md`
- `docs/secrets_and_key_management.md`
- `docs/software_release_checklist.md`

## 硬件联调文档

- `docs/l160_l610_edge_function_upload_quick_start.md`

## 模型文档

- `docs/model_version_and_inference_policy.md`
- `runs/field_tests/README.md`

## 答辩文档

- `docs/project_architecture_formal.md`
- `docs/data_flow_edge_cloud.md`
- `docs/l160_upload_chain_diagram.md`
- `docs/security_design_for_defense.md`
- `docs/demo_script_formal.md`
- `docs/risk_and_backup_plan.md`
- `docs/defense_qa.md`

## 演示入口

- Dashboard：`http://127.0.0.1:8001`
- 正式设备上传 URL：`https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload`

## 截图路径

- `dashboard_edge_function_final_verify.png`
- `dashboard_verify.png`
- `dashboard_e2e_verify.png`
