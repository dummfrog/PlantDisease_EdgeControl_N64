# 风险与备用方案

## 主要风险

- STM32N647 原板售后风险：准备借板或 PC YOLO 演示链路。
- 借板无 LCD 风险：使用串口日志和 Dashboard 投屏证明闭环。
- NPU 未完成风险：PC YOLO 作为对照演示，正式 JSON 上报协议保持一致。
- OV5640 未完成风险：使用样本图片或打印样本卡进行推理演示。
- L160 现场网络风险：提前验证 SIM、信号、PDP、HTTPS。
- Edge Function 部署风险：准备本地 serve 和线上部署验证截图。
- Supabase 网络风险：准备最近历史记录和本地 mock fallback。
- 模型现场样本域偏移风险：准备现场测试 CSV 和误判样例说明。
- Dashboard 本地端口冲突风险：固定 8001，若冲突换端口只影响本地投屏，不影响正式设备入口。

## 备用演示方案

备用方案只能用于现场风险应对，不能写成正式主线：

- PC YOLO 上传图片演示闭环。
- `tools/test_edge_device_upload.py` 模拟 L160/L610 上报。
- 本地 mock fallback 展示 Dashboard。
- Cloudflare Tunnel / ngrok 仅用于应急联调，不作为正式设备入口。
