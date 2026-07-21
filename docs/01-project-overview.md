# 01 项目概述

## 1.1 项目名称

- 中文：DrivePilot 智能座舱模拟系统
- 英文：DrivePilot Cockpit
- 版本：v1.0.0
- 类型：Qt 桌面端智能座舱模拟、DMS 视觉服务、AI Agent 服务
- 开发周期：约 7 天集中迭代

## 1.2 项目背景

传统 HMI 课程项目通常只展示静态页面或简单控件。DrivePilot 在此基础上扩展了媒体、天气、导航、联系人、视频、矢量绘图、驾驶员疲劳监测和 AI Agent，使项目同时体现：

- Qt Quick/QML 复杂 UI；
- C++ 状态管理、网络、模型和本地数据；
- Python FastAPI 服务；
- 计算机视觉训练与 ONNX 部署；
- WebSocket 流式通信；
- 大模型 Tool Calling 与多步 Agent 循环；
- 配置、测试、隐私和工程文档。

## 1.3 建设目标

1. 构建可运行、可演示的车载中控模拟客户端；
2. 将视觉推理和大模型编排从 Qt 客户端解耦；
3. 通过标准协议连接三个独立进程；
4. 保护云服务密钥和个人摄像头数据；
5. 形成可供实习面试讲解的软件工程作品集。

## 1.4 系统边界

系统包含：

- Qt 客户端中的模拟车辆状态；
- 本机摄像头 DMS；
- 网络天气、地图、语音识别和大模型服务；
- 本地 SQLite、QSettings 和媒体文件。

系统不包含：

- 真实 CAN/LIN 总线；
- 真实车辆 ECU 控制；
- 量产级车规 Linux、AUTOSAR 或功能安全认证；
- 医疗或驾驶安全责任判断；
- 云端账号、支付和 OTA 系统。

## 1.5 主要用户

| 用户 | 目标 |
|---|---|
| 驾驶员/演示用户 | 操作中控、使用媒体导航和语音助手 |
| 项目维护者 | 配置服务、运行三个进程、排查日志 |
| 面试官/评审 | 阅读架构、运行演示、查看测试和设计 |
| 后续开发者 | 扩展工具、模型、页面和真实硬件接口 |

## 1.6 技术选型摘要

| 领域 | 技术 |
|---|---|
| 客户端 | Qt 6.9.1、Qt Quick/QML、C++17、CMake |
| 客户端网络 | QNetworkAccessManager、Qt WebSockets、WebEngine、WebChannel |
| 本地数据 | QSettings、SQLite、文件系统 |
| DMS | PyTorch、MobileNetV2、OpenCV YuNet、ONNX Runtime、FastAPI |
| AI Agent | FastAPI、WebSocket、Kimi Tool Calling、Pydantic |
| 第三方服务 | 高德地图、Open-Meteo、讯飞 WebSocket ASR、Kimi |
| 测试 | pytest、模型测试集评估、人工端到端验收 |
