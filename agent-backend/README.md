# DrivePilot AI Agent Backend

独立 FastAPI AI Agent 服务。Qt 负责界面、语音输入和车机工具执行；后端负责 Kimi 调用、会话、工具编排、超时和结果观察。

## Architecture

```text
Qt user input
  → WebSocket
  → AgentRunner
  → Kimi tool_call
  → Qt tool bridge
  → tool_result
  → Kimi continues
  → final response
```

界面只展示面向用户的分析摘要和执行轨迹，不直接公开模型完整隐式思维链。

## Setup

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements-dev.txt
Copy-Item .env.example .env
```

配置：

```env
KIMI_API_KEY=你的真实Key
KIMI_BASE_URL=https://api.moonshot.cn/v1
KIMI_MODEL=kimi-k2.6
KIMI_THINKING_ENABLED=true
KIMI_KEEP_REASONING=true
```

## Test and Run

```powershell
pytest -q
python scripts/run_server.py
```

Without a valid key, the server uses `local-demo`.

## Endpoints

```text
GET http://127.0.0.1:8770/health
GET http://127.0.0.1:8770/api/v1/agent/config
WS  ws://127.0.0.1:8770/ws/agent/{session_id}
```

## Safety

- fixed tool whitelist;
- JSON Schema arguments;
- Qt-side validation;
- tool timeout;
- maximum Agent steps;
- no shell or arbitrary file tool.

See [AGENT_CARD.md](AGENT_CARD.md).
