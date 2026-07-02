# 项目正式架构

## 项目名称

STM32N647 智慧农业病虫害识别与端云协同闭环系统。

## 项目目标

完成从图像采集、边缘/PC 推理、专家处方、执行动作、4G 上传、云端存储、Dashboard 分析到告警展示的闭环演示。

## 总体架构

```text
STM32N647 边缘端
-> OV5640 / NPU / 传感器 / 继电器
-> JSON v1.0
-> L160/L610 4G HTTP 客户端
-> Supabase Edge Function
-> Supabase PostgreSQL
-> Flask Dashboard 8001
```

PC YOLO 是演示和对照链路，正式硬件上传链路以 L160/L610 访问 Edge Function 为主。

## STM32N647 边缘端

STM32N647 负责采集图像、读取传感器、执行继电器动作，并在 NPU 链路可用时完成边缘推理。当前 PC YOLO 可作为演示链路和模型验证工具。

## L160/L610 4G 上传

L160/L610 是 HTTP 客户端，负责把原始扁平 JSON v1.0 POST 到公网 HTTPS Edge Function。它不访问 localhost，不直接写 Supabase REST。

## Supabase Edge Function

`device-upload` 负责 Header 鉴权、HMAC 校验、timestamp 防重放、JSON v1.0 校验、专家规则归一化，并用 service role 写入数据库。

## Supabase PostgreSQL

核心表为 `diagnosis_records`，正式扩展表包括 `device_registry`、`device_telemetry`、`actuation_logs`、`security_events`、`model_versions`。

## Dashboard

本地 Flask Dashboard 运行在 `http://127.0.0.1:8001`，用于投屏展示、PC 图片上传 YOLO 演示、读取 Supabase 和可视化。
