# 软件测试报告模板

## 基本信息

- 测试日期：
- 测试版本：
- 测试人：
- 测试环境：

## 测试结果

| 测试项 | 命令或入口 | 结果 | 备注 |
| --- | --- | --- | --- |
| Python 编译 | `python -m py_compile ...` |  |  |
| 单元测试 | `python -m unittest discover -s tests -p "test_*.py"` |  |  |
| Edge 正常上传 | `tools/test_edge_device_upload.py` |  |  |
| Edge 异常拒绝 | `tools/test_edge_invalid_upload.py` |  |  |
| Dashboard API | `tests/test_dashboard_api.py` |  |  |
| Dashboard E2E | `tools/test_dashboard_e2e.py` |  |  |
| 压力测试 | `tools/load_test_edge_upload.py --count 100` |  |  |

## 问题记录

| 编号 | 问题 | 严重级别 | 处理状态 | 负责人 |
| --- | --- | --- | --- | --- |
|  |  |  |  |  |

## 截图路径

- Dashboard E2E：`dashboard_e2e_verify.png`
- 正式验证截图：

## 结论

- 是否达到验收标准：
- 是否允许发布：
- 遗留风险：
