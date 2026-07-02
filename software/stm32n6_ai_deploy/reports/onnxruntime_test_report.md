# ONNXRuntime Test Report

- Model: `C:\Users\kaxiusi\Desktop\嵌入式\stm32n6_ai_deploy\models\best.onnx`
- Provider: CPUExecutionProvider
- Input name: `images`
- Input shape: `[1, 3, 224, 224]`
- Input dtype: `tensor(float)`
- Resolved preprocessing: RGB, resize 224x224, float32, 0..1 normalization, NCHW
- Outputs: [('output0', [1, 8], 'tensor(float)')]
- Test images: 8
- Softmax applied by script: no
- CSV: `C:\Users\kaxiusi\Desktop\嵌入式\stm32n6_ai_deploy\expected_results\expected_results.csv`

## Results

| image | status | top-1 | confidence | top-3 |
| --- | --- | --- | --- | --- |
| Pepper_bell_Bacterial_spot__0022d6b7-d47c-4ee2-ae9a-392a53f48647___JR_B.Spot 8964.JPG | ok | 0 Pepper_Bell_Bacterial_Spot | 0.999997 | 0:Pepper_Bell_Bacterial_Spot:0.999997 | 5:Tomato_Early_Blight:0.000002 | 1:Pepper_Bell_Healthy:0.000001 |
| Pepper_bell_healthy__016ed5ad-be29-4e9d-8ae5-069a016b1327___JR_HL 8536.JPG | ok | 1 Pepper_Bell_Healthy | 0.999999 | 1:Pepper_Bell_Healthy:0.999999 | 0:Pepper_Bell_Bacterial_Spot:0.000001 | 7:Tomato_Healthy:0.000000 |
| Potato_Early_blight__0182e991-97f0-4805-a1f7-6e1b4306d518___RS_Early.B 7015.JPG | ok | 2 Potato_Early_Blight | 1.000000 | 2:Potato_Early_Blight:1.000000 | 3:Potato_Late_Blight:0.000000 | 6:Tomato_Late_Blight:0.000000 |
| Potato_healthy__170f1f57-0fd4-421f-9c82-3b1804be63ad___RS_HL 1771.JPG | ok | 4 Potato_Healthy | 0.999656 | 4:Potato_Healthy:0.999656 | 3:Potato_Late_Blight:0.000343 | 1:Pepper_Bell_Healthy:0.000001 |
| Potato_Late_blight__0450570b-44d1-4290-956d-5d970164a2e2___RS_LB 5160.JPG | ok | 3 Potato_Late_Blight | 1.000000 | 3:Potato_Late_Blight:1.000000 | 6:Tomato_Late_Blight:0.000000 | 4:Potato_Healthy:0.000000 |
| Tomato_Early_blight__01861c93-ea8b-4820-aaa8-cc6003b3e75b___RS_Erly.B 7855.JPG | ok | 5 Tomato_Early_Blight | 1.000000 | 5:Tomato_Early_Blight:1.000000 | 0:Pepper_Bell_Bacterial_Spot:0.000000 | 2:Potato_Early_Blight:0.000000 |
| Tomato_healthy__02b4afdf-e1de-4c0e-a38d-3f19afeb9ea9___RS_HL 0493.JPG | ok | 7 Tomato_Healthy | 0.999805 | 7:Tomato_Healthy:0.999805 | 5:Tomato_Early_Blight:0.000195 | 6:Tomato_Late_Blight:0.000000 |
| Tomato_Late_blight__013f987a-9371-4763-a104-ea6f326e584b___GHLB2 Leaf 8556.JPG | ok | 6 Tomato_Late_Blight | 1.000000 | 6:Tomato_Late_Blight:1.000000 | 7:Tomato_Healthy:0.000000 | 3:Potato_Late_Blight:0.000000 |

## Risk Notes

- These are host-side ONNXRuntime smoke-test results, not STM32N6 validation results.
- If top-1 labels do not match the filename class, verify training/export preprocessing and whether the ONNX model is detection-style rather than pure classification.
- Hardware firmware must preserve the exact labels order from `labels.txt`.
