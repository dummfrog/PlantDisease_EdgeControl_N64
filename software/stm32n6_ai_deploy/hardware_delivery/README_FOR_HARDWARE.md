# README for Hardware Integration

## Package Purpose

This package provides the current 8-class plant disease classification model for STM32N647 / STM32N6 camera-side integration testing.

## Class Order

The class order is fixed and must match `labels.txt` exactly:

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

## Model I/O

- Model file: `best.onnx`
- Input tensor: `images`
- Input shape: `[1, 3, 224, 224]`
- Layout: NCHW
- Color format: RGB
- Input dtype: FP32
- Normalization: `pixel / 255.0`
- Output tensor: `output0`
- Output shape: `[1, 8]`
- Output handling: run `argmax` to produce `disease_id`; max value is `confidence`.

## Camera Integration Flow

1. OV5640 captures image.
2. Firmware crops/resizes to `224x224`.
3. Firmware converts to RGB.
4. Firmware converts to NCHW tensor.
5. Firmware normalizes to FP32 `0..1`, or applies INT8 quantization parameters when using a future INT8 model.
6. Firmware runs AI inference.
7. Firmware computes `argmax`.
8. Firmware maps `disease_id` to `disease` and `disease_cn`.
9. Firmware fills JSON v1.0 fields:
   - `disease_id`
   - `disease`
   - `disease_cn`
   - `confidence`

## Confidence Policy

- `confidence >= 0.6`: accept result for demo upload.
- `confidence < 0.6`: recapture image or mark for manual review.

## ST Edge AI Status

- STM32N6 CPU/Cortex-M code generation succeeded in `generated_project/`.
- STM32N6 NPU / Neural-ART generation is not complete. Neural-ART analyze failed inside ST `atonn.exe` because the current workspace path contains Chinese characters.
- Current ONNX is FP32. INT8 quantization should be completed before final NPU deployment.

## Included Test Data

- `test_images/`: 8 smoke-test images, one per class.
- `expected_results.csv`: ONNXRuntime top-1 and top-3 reference results.

## Do Not Change

- Do not change labels order.
- Do not feed BGR input as RGB.
- Do not change input size unless the model is retrained or re-exported.
- Do not overwrite `best.onnx`; create `best_int8_stm32n6.onnx` for quantized NPU work.
