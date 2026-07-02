# ST Edge AI Validation Report

## Status

Validation is not complete.

The attempted host validation failed because ST Edge AI does not support host validation for devices based on Arm Cortex-M55/M85:

```text
E102(CliArgumentError): Validation on host is not supported for devices based on Arm Cortex M55/M85 core.
```

## Command Attempted

```powershell
stedgeai validate --target stm32n6 -m .\models\best.onnx -t onnx -w .\stedgeai_output\workspace_validate_cpu -o .\stedgeai_output\validate_cpu --mode host --classifier --batch-size 8 --verbosity 1
```

## Available Reference Result

Host-side ONNXRuntime smoke test succeeded on 8 sample images:

- Report: `reports/onnxruntime_test_report.md`
- CSV: `expected_results/expected_results.csv`

## Required Next Step

Run board target validation after firmware/debug setup is ready:

```powershell
stedgeai validate --target stm32n6 -m .\models\best.onnx -t onnx --mode target --desc serial:COMx:115200 --classifier
```

For NPU validation, replace the model path with the generated INT8 model after Neural-ART analyze/generate succeeds.
