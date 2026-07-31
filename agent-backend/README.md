# DrivePilot AI Agent Backend

这是 DrivePilot Cockpit 的独立 Agent 服务。Qt 负责用户输入、语音识别和车机工具执行；FastAPI 服务负责会话、模型调用、工具编排、超时和结果回传。

## 通信流程

```text
Qt 文本 / 讯飞 ASR 结果
  → WebSocket
  → AgentRunner
  → Kimi tool_call
  → Qt 工具桥
  → tool_result
  → 最终回复
```

界面只显示简要处理状态、工具调用和实际执行结果，不显示模型的隐藏推理内容。

## 安装

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements-dev.txt
Copy-Item .env.example .env
```

在 `.env` 中配置：

```env
KIMI_API_KEY=your_api_key
KIMI_BASE_URL=https://api.moonshot.cn/v1
KIMI_MODEL=kimi-k2.6
KIMI_THINKING_ENABLED=true
KIMI_KEEP_REASONING=true
```

## 测试与运行

```powershell
pytest -q
python scripts/run_server.py
```

未配置有效 Key 时，服务使用本地规则解析少量常用车控指令，便于检查 WebSocket 和工具执行链路。

## 接口

```text
GET http://127.0.0.1:8770/health
GET http://127.0.0.1:8770/api/v1/agent/config
WS  ws://127.0.0.1:8770/ws/agent/{session_id}
```

## 安全边界

- 工具名固定在白名单内；
- 参数使用 JSON Schema；
- Qt 执行前再次校验；
- 单次工具调用有超时限制；
- 不提供 Shell 或任意文件操作工具；
- 云模型密钥只保存在本地 `.env`。
