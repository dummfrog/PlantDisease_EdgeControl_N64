# 端云协同数据流

正式数据流：

```text
图像采集
↓
NPU / PC YOLO 推理
↓
专家知识库
↓
执行动作
↓
JSON v1.0
↓
L160/L610
↓
Edge Function
↓
Supabase PostgreSQL
↓
Dashboard
↓
风险热力图 / 告警 / 历史分析
```

JSON v1.0 body 保持原始结构。安全认证通过 Header 和 Edge Function Secrets 完成。

PC YOLO 链路用于本地演示和模型验证；正式设备上报链路使用 STM32N647 + L160/L610 + Edge Function。
