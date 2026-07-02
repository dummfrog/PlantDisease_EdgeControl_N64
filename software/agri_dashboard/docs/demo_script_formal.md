# 正式演示脚本

1. 打开 `http://127.0.0.1:8001`。
2. 展示系统架构：STM32N647、L160/L610、Edge Function、Supabase、Dashboard。
3. 上传叶片图片，展示 PC YOLO 识别。
4. 展示专家处方、风险等级、泵/风扇动作。
5. 展示 Supabase `diagnosis_records` 记录。
6. 使用 `tools/test_edge_device_upload.py` 展示 Edge Function 设备上传。
7. 展示 L160/L610 正式上报链路图。
8. 刷新 Dashboard 最近诊断记录。
9. 展示风险热力图。
10. 展示告警中心。
11. 展示设备在线状态。
12. 使用异常脚本展示错误 token 被拒绝。
13. 展示低液位禁止喷药逻辑。
14. 总结闭环：感知、诊断、决策、执行、上传、分析。

备用方案只作为风险预案，不替代正式 Edge Function 主线。
