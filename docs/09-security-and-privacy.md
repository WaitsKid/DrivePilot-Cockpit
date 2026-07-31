# 09 安全与隐私设计

## 9.1 保护目标

- 云服务 API Key；
- 驾驶员摄像头画面；
- 个人联系人、通话记录和媒体；
- Agent 会话内容；
- 本地路径和调试日志。

## 9.2 密钥管理

真实凭据只能存放在：

```text
hmi-client/config.json
agent-backend/.env
```

这两个文件均被 `.gitignore` 排除。

仓库只保留：

```text
hmi-client/config.example.json
agent-backend/.env.example
```

禁止：

- 将 Key 写入 C++ 字符串、QML 或 QRC；
- 在截图和日志中展示完整 Key；
- 将 `.env` 复制进 ZIP 或 Release；
- 在 GitHub Issue 中粘贴真实请求头。

如果 Key 曾进入 Git 历史，仅删除文件不够，必须立即在服务控制台轮换 Key，并清理历史。

## 9.3 DMS 隐私

默认配置：

```yaml
privacy:
  store_camera_frames: false
  expose_camera_frames_over_api: false
```

设计边界：

- 摄像头帧只在 DMS Python 进程内存中；
- Qt 只接收概率、时间和状态；
- REST/WebSocket 无图像接口；
- 不默认录像或截图；
- `personal_data/` 不提交。

## 9.4 Agent 安全边界

- 模型只能调用固定工具白名单；
- Qt 再次验证工具名和参数；
- 温度、风量和音量被限制范围；
- 工具具有 `call_id` 和超时；
- Agent 最大执行轮数防止无限循环；
- 模型不能直接访问 QML 对象或系统命令；
- 当前工具只修改模拟状态，不接真实车辆总线。

## 9.5 外部服务风险

| 服务 | 风险 | 控制 |
|---|---|---|
| Kimi | 用户文本发送云端 | 文档告知；不上传摄像头；提交前检查文本内容 |
| 讯飞 | 语音音频发送云端 | 用户主动触发；不保存录音 |
| 高德 | 搜索、位置请求 | 使用最少必要参数；Key 外置 |
| Open-Meteo | 经纬度和天气请求 | 不包含身份信息 |

## 9.6 本地数据

- SQLite 数据库、QSettings 和导入媒体属于运行时数据；
- `.gitignore` 排除 `*.db`、`*.sqlite*`、日志和媒体；
- 提交仓库和录制截图前使用虚构联系人；
- 发截图前对人脸、号码和 Key 打码。

## 9.7 安全声明

本项目不能用于真实驾驶安全判断。任何真实车辆集成必须重新完成威胁分析、功能安全、数据合规、模型验证和系统认证。
