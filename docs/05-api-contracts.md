# 05 接口协议说明

## 5.1 DMS 服务

默认地址：

```text
http://127.0.0.1:8765
```

### REST

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/` | 服务信息 |
| GET | `/health` | 模型、监测和摄像头健康状态 |
| GET | `/api/v1/dms/status` | 当前完整状态 |
| GET | `/api/v1/dms/config` | 可公开的状态机配置 |
| POST | `/api/v1/dms/start` | 启动摄像头监测 |
| POST | `/api/v1/dms/stop` | 停止并释放摄像头 |
| POST | `/api/v1/dms/reset` | 重置时间窗口和状态 |

典型状态：

```json
{
  "service_running": true,
  "models_ready": true,
  "camera_available": true,
  "face_detected": true,
  "fatigue_level": 2,
  "status": "slight_fatigue",
  "status_text": "略微疲劳",
  "closed_probability": 0.81,
  "closed_duration_ms": 1480,
  "perclos": 0.32,
  "yawn_probability": 0.11,
  "yawn_count_window": 0,
  "processed_fps": 8.0,
  "inference_ms": 52.4,
  "event_id": 1,
  "event_type": "slight_fatigue_alert",
  "message": "检测到轻微疲劳，请注意休息。"
}
```

### WebSocket

```text
ws://127.0.0.1:8765/api/v1/dms/events
```

连接后立即推送一次快照，随后只在状态存储修订号变化时推送。

## 5.2 AI Agent 服务

默认地址：

```text
http://127.0.0.1:8770
ws://127.0.0.1:8770/ws/agent/{session_id}
```

### HTTP

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/health` | 服务与模型配置状态 |
| GET | `/api/v1/agent/config` | 模型、思考开关、最大步数和工具超时 |

### Qt → Agent

#### 用户消息

```json
{
  "type": "user_message",
  "content": "把空调调到22度，再播放下一首"
}
```

#### 工具结果

```json
{
  "type": "tool_result",
  "call_id": "call_123",
  "name": "set_ac_temperature",
  "success": true,
  "message": "空调温度已设置为22℃",
  "data": {
    "left_temperature": 22,
    "right_temperature": 22
  }
}
```

#### 控制命令

```json
{"type": "cancel"}
```

```json
{"type": "reset"}
```

```json
{"type": "ping"}
```

### Agent → Qt

| `type` | 说明 |
|---|---|
| `connected` | 会话建立和模型信息 |
| `analysis` | 当前处理状态 |
| `plan` | 待执行操作和工具数量 |
| `tool_call` | 请求 Qt 执行一个工具 |
| `observation` | 工具执行结果 |
| `final` | 最终回复 |
| `error` | 协议、模型或执行错误 |
| `cancelled` | 当前任务已取消 |
| `session_reset` | 会话历史已重置 |
| `pong` | 心跳响应 |
| `done` | 一次任务结束 |

工具调用：

```json
{
  "type": "tool_call",
  "call_id": "call_123",
  "name": "set_ac_temperature",
  "arguments": {
    "temperature": 22,
    "zone": "both"
  },
  "display": "设置空调温度：22℃（both）"
}
```

## 5.3 Agent 工具白名单

| 工具 | 参数 |
|---|---|
| `get_vehicle_state` | 无 |
| `open_page` | `page` |
| `set_ac_temperature` | `temperature`, `zone` |
| `set_ac_fan_level` | `level` |
| `set_ac_mode` | `mode` |
| `control_music` | `action` |
| `set_media_volume` | `volume` |
| `set_vehicle_switch` | `switch_name`, `enabled` |
| `show_toast` | `message` |

## 5.4 Qt 配置

`hmi-client/config.json` 示例：

```json
{
  "xfyun_app_id": "你的讯飞APPID",
  "xfyun_api_key": "你的讯飞APIKey",
  "xfyun_api_secret": "你的讯飞APISecret",
  "amap_web_service_key": "你的高德Web服务Key",
  "amap_default_city": "绍兴",
  "amap_default_longitude": 120.580232,
  "amap_default_latitude": 30.029752,
  "amap_js_key": "你的高德JavaScript Key",
  "amap_js_security_code": "你的高德安全密钥",
  "dms_backend_url": "http://127.0.0.1:8765",
  "dms_voice_enabled": true,
  "ai_agent_backend_url": "ws://127.0.0.1:8770/ws/agent"
}
```

`config.json` 必须保持在 `.gitignore` 中。
