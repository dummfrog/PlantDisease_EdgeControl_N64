# Model Version And Inference Policy

本分支只上传已检查的小型 ONNX 模型、标签文件和模型来源说明。大型训练数据集、完整 `runs/` 输出和未确认模型文件不进入普通 Git。

当前模型文件：

```text
models/best.onnx
models/labels.txt
models/labels_cn.json
```

`best.onnx` 检查结果：约 5.89 MiB，小于 50 MB。仓库已配置 Git LFS 追踪 `*.pt`、`*.onnx`、`*.tflite`、`*.bin`。

模型来源：

```text
C:\Users\kaxiusi\Desktop\嵌入式\plant_disease_yolo\runs\detect\train30_gpu\weights\best.onnx
```

后续如果要上传更大的 `.pt`、`.onnx`、`.tflite` 或 `.bin` 文件，必须先确认 Git LFS 状态、文件大小和模型版本用途。
