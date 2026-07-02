# STM32N6 AI Deployment Summary

## 1. Model Files

- `models/best.onnx`: exists
- `models/best.pt`: exists
- Source copied from stable training run: `plant_disease_yolo/runs/detect/train30_gpu/weights/`
- Original model files were not moved or overwritten.

## 2. ONNX Inspection

- Input: `images`, shape `[1, 3, 224, 224]`, dtype `FLOAT`
- Output: `output0`, shape `[1, 8]`, dtype `FLOAT`
- Class count: 8, PASS
- Dynamic shape: no
- Opset: `ai.onnx:13`
- Operator types: `Conv`, `Sigmoid`, `Mul`, `Add`, `Concat`, `Split`, `Reshape`, `Transpose`, `MatMul`, `Softmax`, `GlobalAveragePool`, `Flatten`, `Gemm`, `Constant`
- Quantization: not quantized; all weights are FP32

## 3. ONNXRuntime Test

- Test image count: 8
- Result: all 8 sample filenames matched the ONNXRuntime top-1 class.
- Reference outputs:
  - `expected_results/expected_results.csv`
  - `reports/onnxruntime_test_report.md`

## 4. ST Edge AI Toolchain

- Python: `3.10.11`
- ONNX: `1.22.0`
- ONNXRuntime: `1.23.2`
- Ultralytics: `8.4.76`
- NumPy: `2.2.6`
- Pillow: `12.2.0`
- Pandas: not installed, not required by the generated scripts
- `stedgeai`: available
- `stedgeai` version: ST Edge AI Core `v4.0.1-20581`
- `stm32ai`: not found in PATH
- `STM32CubeMX`: not found in PATH

## 5. Analyze Result

- STM32N6 CPU/Cortex-M analyze: success
- STM32N6 NPU / Neural-ART analyze: failed due ST compiler path-encoding error under the Chinese workspace path
- Unsupported ops: none explicitly reported before the Neural-ART compiler failure
- Memory from CPU analyze:
  - Weights: `6,145,144 B` (`5.86 MiB`)
  - Activations: `2,007,040 B` (`1.91 MiB`)
  - Total FLASH/RO summary: `6,200,318 B`
  - Total RAM/RW summary: `2,027,176 B`

## 6. Generate Result

- STM32N6 CPU/Cortex-M generate: success
- Output path: `generated_project/`
- Key files:
  - `network.c`
  - `network.h`
  - `network_data.c`
  - `network_data.h`
  - `network_details.h`
  - `network_c_info.json`
  - `network_generate_report.txt`
- STM32N6 Neural-ART/NPU generate: not executed because Neural-ART analyze did not succeed.

## 7. Validate Result

- Host validation: failed by CLI limitation
- Error: `Validation on host is not supported for devices based on Arm Cortex M55/M85 core.`
- Required next step: target validation on board with serial connection.

## 8. Hardware Handoff Status

Current model can be handed to hardware classmates for camera preprocessing and CPU-path STM32N6 integration reference testing.

It should not yet be claimed as NPU-ready. Missing items for final NPU claim:

- INT8 quantized ONNX model, suggested path `models/best_int8_stm32n6.onnx`
- Neural-ART analyze/generate from an ASCII-only path
- Board target validation with real serial connection

## 9. Next Commands

After copying this package to an ASCII-only path:

```powershell
stedgeai analyze --target stm32n6 --st-neural-art -m .\models\best_int8_stm32n6.onnx -t onnx -w .\stedgeai_output\workspace_npu_int8 -o .\stedgeai_output\analyze_npu_int8 --verbosity 1
```

For board validation:

```powershell
stedgeai validate --target stm32n6 -m .\models\best_int8_stm32n6.onnx -t onnx --mode target --desc serial:COMx:115200 --classifier
```

For L160/L610 upload integration, keep JSON v1.0 unchanged and fill only:

```text
disease_id
disease
disease_cn
confidence
```
