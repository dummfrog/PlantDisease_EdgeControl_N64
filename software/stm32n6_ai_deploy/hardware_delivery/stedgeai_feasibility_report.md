# ST Edge AI Feasibility Report

## Conclusion

- STM32N6 target availability: yes, `stedgeai` lists `stm32n6 [--st-neural-art]`.
- STM32CubeAI / CPU path: analyze succeeded and generate succeeded.
- STM32N6 NPU / Neural-ART path: not yet deliverable. The Neural-ART compiler failed inside `atonn.exe` when the workspace path contained non-ASCII characters from `C:\Users\kaxiusi\Desktop\嵌入式`.
- Unsupported ops: no explicit unsupported ONNX operator was reported before the Neural-ART compiler failure.
- Quantization status: current ONNX is FP32 and unquantized. Neural-ART emitted many `is not quantized` warnings.
- Board-ready status: CPU generated code can be handed to hardware for reference integration; NPU deployment still requires an ASCII-path rerun outside the Chinese path or a fixed ST toolchain workflow, plus INT8 quantization evaluation.

## Toolchain

- `stedgeai`: `D:\ST\STEdgeAI\4.0\Utilities\windows\stedgeai.exe`
- Version: ST Edge AI Core `v4.0.1-20581`
- Components reported by CLI:
  - ISPU `2.0.1-RC2`
  - MLC `1.2.4-RC2`
  - StellarAI `4.0.1-RC2`
  - STM32CubeAI `12.0.1-RC2`
- `stm32ai`: not found in PATH
- `STM32CubeMX`: not found in PATH

## Model

- Model: `models/best.onnx`
- Input: `images`, `float32`, shape `[1, 3, 224, 224]`
- Output: `output0`, `float32`, shape `[1, 8]`
- Classes: 8
- Opset: `ai.onnx:13`
- Dynamic shape: no
- Quantized: no

## STM32N6 Analyze Result Without Neural-ART

Command:

```powershell
stedgeai analyze --target stm32n6 -m .\models\best.onnx -t onnx -w .\stedgeai_output\workspace_analyze_cpu -o .\stedgeai_output\analyze_cpu --verbosity 1
```

Result: success.

Key memory figures from ST Edge AI:

- Parameters: `1,536,286` items, `5.86 MiB`
- MACC: `262,172,798`
- Weights: `6,145,144 B` (`5.86 MiB`)
- Activations: `2,007,040 B` (`1.91 MiB`)
- Total RAM reported for model buffers: `2,007,040 B`
- Total generated target footprint summary:
  - FLASH/RO total: `6,200,318 B`
  - RAM/RW total: `2,027,176 B`

Report files:

- `stedgeai_output/analyze_cpu/network_analyze_report.txt`
- `stedgeai_output/analyze_cpu/network_c_info.json`
- `reports/stedgeai_analyze_cpu_log.txt`

## Neural-ART / NPU Analyze Result

Command attempted:

```powershell
stedgeai analyze --target stm32n6 --st-neural-art -m .\models\best.onnx -t onnx -w .\stedgeai_output\workspace_analyze -o .\stedgeai_output\analyze --verbosity 1
```

Result: failed.

Observed failure:

```text
E103(CliRuntimeError): Error calling the Neural Art compiler
terminate called after throwing an instance of 'std::range_error'
what():  wstring_convert::from_bytes
Internal compiler error (signo=22)
```

A second attempt used a temporary `N:` drive mapping so the model input path became ASCII-like. The compiler still generated internal output paths under the original Chinese path and failed with:

```text
std::filesystem::__cxx11::filesystem_error
what(): filesystem error: Cannot convert character sequence: Illegal byte sequence
```

Interpretation: this is a toolchain/path-encoding blocker before a reliable NPU feasibility decision can be made. It does not prove the model is unsupported by Neural-ART; it means this workspace path cannot currently complete Neural-ART compilation.

## Generate Result

Command:

```powershell
stedgeai generate --target stm32n6 -m .\models\best.onnx -t onnx -w .\stedgeai_output\workspace_generate_cpu -o .\generated_project --verbosity 1
```

Result: success for STM32N6 CPU/Cortex-M path.

Generated key files:

- `generated_project/network.c`
- `generated_project/network.h`
- `generated_project/network_data.c`
- `generated_project/network_data.h`
- `generated_project/network_details.h`
- `generated_project/network_c_info.json`
- `generated_project/network_generate_report.txt`

## Validate Result

Command attempted:

```powershell
stedgeai validate --target stm32n6 -m .\models\best.onnx -t onnx -w .\stedgeai_output\workspace_validate_cpu -o .\stedgeai_output\validate_cpu --mode host --classifier --batch-size 8 --verbosity 1
```

Result: failed because the CLI does not support host validation for this core family:

```text
E102(CliArgumentError): Validation on host is not supported for devices based on Arm Cortex M55/M85 core.
```

Next validation step requires board-side target validation with serial connection parameters.

## Required Next Steps

1. Move or clone this `stm32n6_ai_deploy` package to an ASCII-only path, for example `C:\stm32n6_ai_deploy`, then rerun Neural-ART analyze/generate.
2. Build an INT8 quantized ONNX for NPU evaluation; do not overwrite `models/best.onnx`.
3. Rerun:

```powershell
stedgeai analyze --target stm32n6 --st-neural-art -m .\models\best_int8_stm32n6.onnx -t onnx -w .\stedgeai_output\workspace_npu_int8 -o .\stedgeai_output\analyze_npu_int8 --verbosity 1
```

4. After NPU generate succeeds, run board target validation with the actual COM port:

```powershell
stedgeai validate --target stm32n6 -m .\models\best_int8_stm32n6.onnx -t onnx --mode target --desc serial:COMx:115200 --classifier
```
