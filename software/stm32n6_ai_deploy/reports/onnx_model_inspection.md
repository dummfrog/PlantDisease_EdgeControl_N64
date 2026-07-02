# ONNX Model Inspection

- Model: `C:\Users\kaxiusi\Desktop\嵌入式\stm32n6_ai_deploy\models\best.onnx`
- File size: 6173966 bytes
- ONNX opset: ai.onnx:13
- Graph nodes: 153
- Initializers: 80
- Dynamic shape: no
- Detected output class count: 8
- Expected class count: 8
- Class count check: PASS
- Quantization ops present: no
- Tensor value-info dtypes: {'FLOAT': 2}
- Weight dtypes: {'FLOAT': 80}

## Inputs

- `images` shape=[1, 3, 224, 224] dtype=FLOAT

## Outputs

- `output0` shape=[1, 8] dtype=FLOAT

## Operator Types

- Add: 9
- Concat: 7
- Constant: 9
- Conv: 39
- Flatten: 1
- Gemm: 1
- GlobalAveragePool: 1
- MatMul: 2
- Mul: 36
- Reshape: 3
- Sigmoid: 35
- Softmax: 2
- Split: 6
- Transpose: 2

## STM32N6 Deployment Notes

- This script only validates ONNX structure and local ONNXRuntime readiness.
- Final STM32N6/NPU support must be confirmed by ST Edge AI analyze/generate.
- If the model remains FP32, INT8 quantization should be evaluated before NPU deployment.
