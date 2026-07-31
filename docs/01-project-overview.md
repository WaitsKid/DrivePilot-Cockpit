# 项目概述

## 项目范围

DrivePilot Cockpit 是我基于 Qt Quick / QML 和 C++ 完成的桌面端智能座舱模拟系统。项目同时包含两个 Python 服务：一个处理驾驶员疲劳监测，一个处理大模型工具调用。

我把界面、车辆状态和工具执行放在 Qt 客户端，把摄像头推理和模型调用拆成独立进程。这样 Qt 不需要直接依赖 PyTorch、OpenCV 或云模型 SDK，Python 服务离线时也不会影响其他页面使用。

## 主要组成

- **Qt 客户端**：页面路由、空调与车辆状态、音乐视频、天气地图、联系人、计算器和矢量画板；
- **DMS 服务**：YuNet 人脸检测、MobileNetV2 眼睛/哈欠分类、ONNX 推理和疲劳状态机；
- **Agent 服务**：WebSocket 会话、Kimi Tool Calling、工具等待、超时和结果回传；
- **外部服务**：讯飞语音识别、高德地图和 Open-Meteo 天气。

## 技术选型

| 部分 | 技术 |
|---|---|
| 客户端 | Qt 6.9.1、Qt Quick/QML、C++17、CMake |
| 客户端网络 | QNetworkAccessManager、Qt WebSockets、WebEngine、WebChannel |
| 本地数据 | SQLite、QSettings、文件系统 |
| DMS | PyTorch、MobileNetV2、OpenCV YuNet、ONNX Runtime、FastAPI |
| Agent | FastAPI、WebSocket、Kimi Tool Calling、Pydantic |
| 测试 | pytest、测试集评估、端到端联调 |

## 系统边界

项目中的车辆状态和控制均为软件模拟，不执行真实车辆 ECU 操作。系统不包含真实 CAN/LIN 总线、AUTOSAR、OTA、车规 Linux 或功能安全认证。摄像头数据默认只在 DMS 进程中处理，不保存也不通过 API 返回。
