# STM32N647 智慧农业云平台 Dashboard

这个 Dashboard 用于演示完整软件闭环：

```text
上传叶片图片
-> YOLO 分类推理
-> 查询专家知识库
-> 生成处方建议和继电器动作
-> 写入 Supabase PostgreSQL
-> Web Dashboard 可视化展示
```

## 目录

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard
├─ backend\app.py              Flask API
├─ backend\database.py         Supabase、YOLO、知识库、mock fallback
├─ frontend\index.html         原生 JS + Chart.js 单页 Dashboard
├─ supabase\functions\device-upload\index.ts
├─ supabase\migrations\001_formal_schema_extension.sql
├─ supabase\rls_formal_policies.sql
├─ tests\
├─ scripts\
├─ tools\test_edge_device_upload.py
├─ docs\supabase_edge_function_device_upload_deploy.md
├─ docs\l160_l610_edge_function_upload_quick_start.md
├─ supabase_schema.sql         Supabase PostgreSQL 建表脚本
├─ requirements.txt
└─ .env
```

## 正式版架构

```text
正式设备上报入口：Supabase Edge Function device-upload
本地 Dashboard / PC YOLO 演示入口：http://127.0.0.1:8001
L160/L610 上传 URL：https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

正式链路：

```text
STM32N647 + L160/L610
-> Supabase Edge Function: device-upload
-> Supabase PostgreSQL: diagnosis_records
-> Dashboard
```

本地 Flask 后端的角色：

1. 本地 Dashboard 页面服务。
2. PC 端图片上传 YOLO 推理演示。
3. 读取 Supabase 进行可视化展示。
4. 不再作为 L160/L610 正式公网入口。

Cloudflare Tunnel / ngrok 只作为备选调试方案，不是当前主方案。

## 环境安装

```powershell
cd C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard
python -m pip install -r requirements.txt
```

如果 `ultralytics` 或 `torch` 已经在你的 YOLO 虚拟环境里安装，也可以直接用那个虚拟环境运行 Dashboard。

## `.env` 配置

```env
SUPABASE_URL=https://你的项目.supabase.co
SUPABASE_KEY=你的 anon key
SUPABASE_TABLE=diagnosis_records
SUPABASE_ACTION_TABLE=action_records

YOLO_MODEL_PATH=C:\Users\kaxiusi\Desktop\嵌入式\plant_disease_yolo\runs\detect\train30_gpu\weights\best.pt
YOLO_IMGSZ=224
```

说明：

- 后端只在本机 Flask 服务中使用 Supabase key，前端不会暴露 key。
- Supabase service role key 只允许放在 Edge Function Secrets，不允许写入前端或硬件端示例。
- 设备端只保存对应节点自己的设备 token 和 HMAC secret。
- 当前项目保留 mock fallback：Supabase 未配置、表权限失败或 YOLO 模型不可用时，页面仍可演示。
- 如果后续换新权重，只需要改 `YOLO_MODEL_PATH`。

## Supabase 配置

如果 `diagnosis_records` / `action_records` 尚未建立，在 Supabase 网页 SQL Editor 执行：

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard\supabase_schema.sql
```

当前代码默认读写：

- `public.diagnosis_records`
- `public.action_records`

脚本已经包含 RLS、`anon/authenticated` 的 `select/insert` policy 和 Data API 权限。比赛演示阶段可直接用；正式项目应改成更严格的设备鉴权。

正式 RLS 策略位置：

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard\supabase\rls_formal_policies.sql
```

正式策略会撤销演示阶段过宽的匿名 insert；设备写入统一由 Edge Function 使用 service role 完成。

## 运行

```powershell
cd C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard
scripts\start_dashboard_8001.bat
```

浏览器打开：

```text
http://127.0.0.1:8001
```

## API

- `GET /api/health`：服务、Supabase、YOLO 模型状态
- `GET /api/stats`：总览统计、分布图、趋势图数据
- `GET /api/history`：历史诊断记录
- `GET /api/devices`：设备在线状态
- `GET /api/node_status`：Node01/Node02/Node03 正式状态卡数据
- `GET /api/alerts`：告警中心数据
- `GET /api/heatmap_matrix`：最近 24 小时节点热力图
- `GET /api/model/status`：模型路径、类别映射、ONNX 状态
- `GET /api/export_csv`：导出当前筛选记录，不包含任何密钥
- `POST /api/upload`：上传图片并执行完整闭环
- `POST /api/predict`：只推理，不写数据库
- `POST /api/simulate_device`：模拟 STM32/NPU 上传一条记录

## Edge Function 正式设备上报

Edge Function 源码：

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard\supabase\functions\device-upload\index.ts
```

部署文档：

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard\docs\supabase_edge_function_device_upload_deploy.md
```

硬件联调文档：

```text
C:\Users\kaxiusi\Desktop\嵌入式\agri_dashboard\docs\l160_l610_edge_function_upload_quick_start.md
```

本地/线上测试脚本：

```powershell
python tools\test_edge_device_upload.py http://127.0.0.1:54321/functions/v1/device-upload
python tools\test_edge_device_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
python tools\test_edge_invalid_upload.py https://xodxtuctgknnftfgoray.functions.supabase.co/device-upload
```

设备认证使用 HTTP Header：

```text
X-Device-Id
X-Device-Token
X-Timestamp
X-Signature
```

`X-Signature` 是 JSON body 原文的 HMAC-SHA256 十六进制签名，JSON v1.0 body 不新增认证字段。

## 正式测试

```powershell
scripts\run_local_tests.bat
python -m unittest discover -s tests -p "test_*.py"
python tools\test_dashboard_e2e.py --url http://127.0.0.1:8001
python tools\load_test_edge_upload.py http://127.0.0.1:54321/functions/v1/device-upload --count 100
```

## 正式部署

```powershell
supabase login
supabase link --project-ref xodxtuctgknnftfgoray
supabase functions serve device-upload --no-verify-jwt --env-file .env
supabase functions deploy device-upload --no-verify-jwt
```

RLS 正式切换：

```text
先执行 supabase/migrations/001_formal_schema_extension.sql
如已执行过 001，则继续执行 supabase/migrations/002_hardware_status_fields.sql
再执行 supabase/rls_formal_policies.sql
```

正式 RLS 会禁止匿名 insert。Edge Function 使用 service role 写入，Dashboard 只读。

## 文档索引

- `docs/database_schema_formal.md`：正式数据库结构、索引、视图和 RLS 执行顺序。
- `docs/security_model.md`：正式安全模型。
- `docs/software_test_plan.md`：测试计划。
- `docs/software_test_report_template.md`：测试报告模板。
- `docs/formal_deployment_guide.md`：正式部署指南。
- `docs/supabase_edge_function_device_upload_deploy.md`：Edge Function 部署说明。
- `docs/secrets_and_key_management.md`：Secrets 管理。
- `docs/software_release_checklist.md`：发布检查清单。
- `docs/l160_l610_edge_function_upload_quick_start.md`：硬件联调快速开始。
- `docs/model_version_and_inference_policy.md`：模型版本与推理策略。
- `docs/project_architecture_formal.md`：项目正式架构。
- `docs/data_flow_edge_cloud.md`：端云数据流。
- `docs/l160_upload_chain_diagram.md`：L160/L610 上报链路。
- `docs/security_design_for_defense.md`：答辩安全设计。
- `docs/demo_script_formal.md`：正式演示脚本。
- `docs/risk_and_backup_plan.md`：风险与备用方案。
- `docs/defense_qa.md`：答辩 Q&A。
- `docs/final_delivery_index.md`：最终交付索引。

## 演示流程

1. 启动 Flask：`python backend\app.py`
2. 打开 `http://127.0.0.1:8001`
3. 确认右上角显示 `API 正常 / Supabase 已配置`
4. 点击 `模拟 STM32 上传`，观察总览、图表、设备状态和历史记录刷新
5. 上传叶片图片，观察 YOLO 识别结果、专家处方和 Supabase 写入记录

## 注意

实际检查发现当前 `best.pt` 的模型类别名是 `Pepper/Potato/Tomato` 旧 8 类。后端已做动态归一化和知识库别名兼容；如果换成包含 `Tomato_Leaf_Mold`、`Tomato_Septoria_Leaf_Spot` 的新 8 类权重，Dashboard 不需要改前端。

正式设备端当前 8 类映射：

```text
0 Pepper_Bell_Bacterial_Spot
1 Pepper_Bell_Healthy
2 Potato_Early_Blight
3 Potato_Late_Blight
4 Potato_Healthy
5 Tomato_Early_Blight
6 Tomato_Late_Blight
7 Tomato_Healthy
```

## 当前硬件字段支持

设备 JSON v1.0 支持以下硬件状态字段，仍然保持扁平 body：

- `liquid_level`：可继续上传占位 `OK`。
- `soil_moisture_percent`：可继续上传 mock `42`。
- `current_ma`：用于 INA219 真实电流。
- `system_status`：用于设备在线、启动、错误等状态。
- `sensor_status`：用于传感器健康状态。
- `pump_action` / `fan_action`：支持 `ON`、`OFF`、`NONE`、`REQUEST`。当前泵/风扇只记录请求、不真实启动时，上传 `REQUEST`。

线上库如果已经执行过 `001_formal_schema_extension.sql`，需要继续执行：

```text
supabase/migrations/002_hardware_status_fields.sql
```
