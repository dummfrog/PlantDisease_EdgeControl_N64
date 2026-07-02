# 模型版本与推理策略

## 当前模型链路

本地 Dashboard 的 PC YOLO 演示通过 `/api/upload` 上传图片，后端读取 `.env` 中的 `YOLO_MODEL_PATH`。如果模型文件不存在或运行环境不具备推理依赖，后端会进入 mock fallback 用于演示页面稳定性。

正式硬件上报链路不上传图片，不调用本地 YOLO。STM32N647/NPU 或设备端结果通过 JSON v1.0 上报到 Edge Function。

## 8 类类别映射

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

替换模型时必须确认类别顺序一致，否则 `disease_id` 到中文名、风险和处方建议的映射会错误。

## best.pt 与 best.onnx

- `best.pt`：PC 端 YOLO 演示默认权重。
- `best.onnx`：可作为导出与嵌入式部署检查产物。
- STM32N647 NPU：正式边缘推理目标，设备端只上传推理后的 JSON v1.0。

## 为什么不立即重训

当前交付目标是完成端云闭环、正式设备入口、安全策略、Dashboard 和答辩材料。模型重训需要稳定样本集、标注和现场验证，适合作为后续迭代，不应阻塞正式链路交付。

## 低置信度策略

`confidence < 0.6` 时：

- 禁止自动喷药。
- 建议重新采集或人工复核。
- 告警标记为 `LOW_CONFIDENCE`。
- Dashboard 告警中心显示复核建议。

## 模型替换流程

1. 准备新权重。
2. 核对 8 类类别顺序。
3. 更新 `.env` 中 `YOLO_MODEL_PATH`。
4. 运行 `GET /api/model/status`。
5. 使用 `tools/model_field_test_logger.py` 批量测试现场样本。
6. 检查误判样例和低置信度样例。
7. 更新 `model_versions` 记录和本文件。

## 现场测试

```powershell
python tools\model_field_test_logger.py C:\path\to\field_images --api-url http://127.0.0.1:8001/api/upload
```

输出 CSV 包括：

- `image_name`
- `predicted_id`
- `predicted_class`
- `confidence`
- `latency_ms`
- `expected_class`
- `correct`

测试样本应覆盖打印样本卡、手机拍摄、弱光、反光、倾斜、距离变化和背景干扰。
