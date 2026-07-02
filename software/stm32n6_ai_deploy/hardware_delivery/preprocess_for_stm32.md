# STM32 Preprocessing Contract

## Input

- Tensor name: `images`
- Shape: `[1, 3, 224, 224]`
- Layout: NCHW
- Color: RGB
- Data type before quantization: `float32`
- Normalization: convert pixel values from `0..255` to `0..1`

## Camera Pipeline

1. Capture frame from OV5640.
2. Crop or resize image to `224x224`.
3. Convert camera output to RGB.
4. Convert to tensor layout NCHW: channel first.
5. Normalize FP32 model input with `value / 255.0`.
6. For future INT8 NPU model, apply the quantization scale/zero-point generated for that model instead of FP32 normalization.
7. Call AI inference.
8. Read 8 output values from `output0`.
9. Run `argmax` to get `disease_id`.
10. Map `disease_id` through `labels.txt` and `labels_cn.json`.
11. Fill JSON v1.0 fields:
    - `disease_id`
    - `disease`
    - `disease_cn`
    - `confidence`

## Output

- Tensor name: `output0`
- Shape: `[1, 8]`
- Meaning: 8 class probabilities or logits. The exported model includes `Softmax`, and ONNXRuntime output behaves like probabilities.
- Firmware action: run `argmax`; confidence is the max output value.

## Low Confidence Rule

If `confidence < 0.6`, the firmware or upper system should mark the result as low confidence and request recapture or manual review.

## Failure Modes to Avoid

- Do not use BGR as RGB.
- Do not change labels order.
- Do not change input size without retraining/exporting the model.
- Do not skip normalization for the FP32 model.
- Do not assume this FP32 model is already NPU-ready.
