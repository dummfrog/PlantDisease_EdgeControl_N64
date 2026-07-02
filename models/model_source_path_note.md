# Model Source Path Note

本次根目录模型来源：

```text
C:\Users\kaxiusi\Desktop\嵌入式\plant_disease_yolo\runs\detect\train30_gpu\weights\best.onnx
```

检查结果：

```text
best.onnx 6173966 bytes
best.pt   3214164 bytes
```

`best.onnx` 小于 50 MB，且仓库已启用 Git LFS 追踪模型扩展名，因此本次将 `best.onnx` 纳入 `models/`。`best.pt` 未放入根 `models/`，避免重复上传和扩大审查范围。
