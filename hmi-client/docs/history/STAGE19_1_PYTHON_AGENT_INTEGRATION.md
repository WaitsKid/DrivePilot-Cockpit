# Stage 19.1：Qt + Python FastAPI + Kimi AI Agent

## 架构调整

Qt 不再直接保存 Kimi API Key，也不再直接请求 Kimi。

```text
Qt 输入/讯飞 ASR
       │ WebSocket
       ▼
DrivePilot AI Agent Backend
       │ Kimi tool_calls
       ▼
Qt AiAgentToolBridge
       │ tool_result
       └────────► Python 继续下一轮推理
```

## Qt 配置

在现有 `config.json` 中加入：

```json
"ai_agent_backend_url": "ws://127.0.0.1:8770/ws/agent"
```

Kimi Key 只放在 Python 后端 `.env`，不要放进 Qt 项目。

## 启动顺序

1. 在 PyCharm 启动：

```bash
python scripts/run_server.py
```

2. 启动 Qt 中控屏。
3. 进入 AI Agent 页面，顶部应显示 `kimi-k2.6` 或 `local-demo`。

## 测试指令

```text
把空调调到22度，切换自动模式，再打开空调页面
播放下一首音乐，把音量调到45
关闭疲劳驾驶监测并打开车辆设置
```

## 事件展示

界面会依次显示：

```text
分析摘要
执行计划
工具调用
真实执行结果
最终回复
```

不展示原始隐式思维链，只展示可审计的安全分析摘要与执行轨迹。
