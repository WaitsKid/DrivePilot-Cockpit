# Stage 5：FastAPI 后台常驻服务与四级疲劳状态机

## 1. 本阶段目标

Stage 4 已经完成单帧视觉信号：

```text
YuNet 人脸检测
  -> 左右眼 ROI
  -> Closed / Open
  -> 完整人脸 ROI
  -> yawn / no_yawn
```

Stage 5 将单帧信号变成连续状态：

```text
单帧闭眼与哈欠概率
  -> 持续闭眼时间
  -> 时间加权 PERCLOS
  -> 哈欠事件计数
  -> 状态升级 / 恢复防抖
  -> 活力 / 正常 / 略微疲劳 / 严重疲劳
  -> FastAPI JSON
```

摄像头属于 Python 后台服务。Qt 不显示、不保存，也不接收驾驶员画面。

## 2. 目录结构

```text
configs/stage5.yaml

dms_backend/
├── fatigue/
│   ├── config.py
│   ├── engine.py
│   └── types.py
├── service/
│   ├── background_service.py
│   └── status_store.py
└── api/
    └── app.py

scripts/
├── 15_validate_stage5.py
├── 16_run_server.py
├── 17_check_service.py
└── 18_simulate_state_machine.py
```

## 3. 安装依赖

继续使用当前项目 `.venv`：

```bash
pip install -r requirements-stage5.txt
```

Stage 5 新增 FastAPI、Uvicorn 和 API 测试所需的 HTTPX。

## 4. 验证模型和服务配置

```bash
python scripts/15_validate_stage5.py
```

这个脚本会：

- 检查 YuNet 模型；
- 检查两个 MobileNetV2 ONNX；
- 检查 metadata 和 deployment config；
- 初始化完整推理管线；
- 验证隐私配置；
- 不打开摄像头。

预期结尾：

```text
Model files: PASS
Model initialization: PASS
Fatigue engine: PASS
Stage 5 validation: PASS
No camera was opened by this validation script.
```

## 5. 先测试状态机

```bash
python scripts/18_simulate_state_machine.py
```

该脚本不使用摄像头，通过模拟闭眼、恢复、严重闭眼和无人脸场景检查状态切换。

## 6. 启动后台服务

```bash
python scripts/16_run_server.py
```

默认地址：

```text
http://127.0.0.1:8765
```

浏览器可打开：

```text
http://127.0.0.1:8765/docs
```

服务启动后不会出现摄像头预览窗口。摄像头画面只在 Python 进程内送入模型。

## 7. 检查实时状态

另开一个 Terminal：

```bash
python scripts/17_check_service.py --watch-seconds 30
```

终端会每秒打印：

```text
level=1 status=状态正常 face=True closed=0.06 perclos=0.03 yawns=0 fps=8.0
```

发生新告警时会打印：

```text
NEW ALERT: 1 slight_fatigue_alert 检测到轻微疲劳，请注意休息。
```

## 8. 四级状态规则

### 0：活力

满足较长时间稳定跟踪、低 PERCLOS、无哈欠、当前眼睛睁开后进入。系统启动后默认先处于“正常”，不会立刻显示“活力”。

### 1：正常

没有达到疲劳条件时的默认状态。

### 2：略微疲劳

任意条件达到即可成为原始候选：

- 持续闭眼约 1.1 秒；
- 30 秒时间窗 PERCLOS 达到 30%；
- 60 秒内确认 2 次哈欠。

候选持续约 0.6 秒后正式升级，避免单帧抖动。

### 3：严重疲劳

任意条件达到即可成为原始候选：

- 持续闭眼约 2.5 秒；
- 30 秒时间窗 PERCLOS 达到 52%；
- 60 秒内确认 3 次哈欠。

候选持续约 0.3 秒后正式升级。

以上参数均在 `configs/stage5.yaml` 中，当前先用默认值完成联调，暂时不要随意修改。

## 9. 防抖与恢复

升级较快：

```text
正常 -> 略微疲劳：约 0.6 秒确认
略微疲劳 -> 严重疲劳：约 0.3 秒确认
```

恢复较慢：

```text
严重疲劳 -> 略微疲劳：持续改善约 10 秒
略微疲劳 -> 正常：持续改善约 8 秒
正常 -> 活力：持续良好约 15 秒
```

自然眨眼不会立刻结束闭眼事件，系统允许约 0.22 秒的短暂信号波动。

## 10. 哈欠事件

哈欠模型连续判定约 0.8 秒才计为一次哈欠。一次哈欠持续数秒也只计数一次，直到信号恢复至少约 0.45 秒才允许计入下一次。

## 11. 无人脸处理

短时间没有检测到人脸：

- 不产生疲劳告警；
- 状态返回 `monitoring_state = no_face`；
- `face_detected = false`；
- 暂时保留最近等级。

连续约 12 秒没有人脸后：

- 清空 PERCLOS 时间窗；
- 清空哈欠计数；
- 状态恢复为正常；
- 避免驾驶员离开后残留严重疲劳状态。

## 12. 告警去重

Qt 后续不要根据 `fatigue_level` 每次轮询都播报，而是比较：

```json
{
  "event_id": 3,
  "event_type": "severe_fatigue_alert",
  "message": "警告，检测到严重疲劳，请立即停车休息。"
}
```

只有 `event_id` 发生变化时，Qt 才显示新的 Toast 并进行语音播报。

持续处于略微疲劳时，默认 45 秒最多提醒一次；严重疲劳默认 20 秒最多提醒一次。

## 13. API

### 健康检查

```http
GET /health
```

### 当前状态

```http
GET /api/v1/dms/status
```

核心返回：

```json
{
  "service_running": true,
  "models_ready": true,
  "camera_available": true,
  "face_detected": true,
  "fatigue_level": 2,
  "status": "slight_fatigue",
  "status_text": "略微疲劳",
  "closed_probability": 0.76,
  "closed_duration_ms": 1430,
  "perclos": 0.31,
  "yawn_count_window": 1,
  "event_id": 1,
  "event_type": "slight_fatigue_alert",
  "message": "检测到轻微疲劳，请注意休息。"
}
```

### 启停与复位

```http
POST /api/v1/dms/start
POST /api/v1/dms/stop
POST /api/v1/dms/reset
```

### WebSocket

```text
ws://127.0.0.1:8765/api/v1/dms/events
```

Qt Stage 6 可以先用 `QNetworkAccessManager` 每 500 ms 轮询状态；稳定后再选择是否使用 WebSocket。轮询更容易调试，当前性能负担很小。

## 14. 隐私设计

本阶段没有：

- 上传图片接口；
- 摄像头预览接口；
- Base64 图像字段；
- 自动截图；
- 自动录像；
- 将画面发送给 Qt 的功能。

`GET /api/v1/dms/config` 会明确返回：

```json
{
  "privacy": {
    "store_camera_frames": false,
    "expose_camera_frames_over_api": false
  }
}
```

## 15. 本阶段验收

1. `python scripts/15_validate_stage5.py` 通过；
2. `python scripts/18_simulate_state_machine.py` 能看到 0～3 状态切换；
3. `python scripts/16_run_server.py` 启动后没有摄像头画面窗口；
4. `/health` 返回模型和摄像头状态；
5. 正常注视时 `fatigue_level` 为 0 或 1；
6. 持续闭眼约 1～2 秒后进入 2；
7. 持续闭眼约 3 秒后进入 3；
8. 新告警只在 `event_id` 增加时触发；
9. 停止服务后摄像头被释放；
10. `pytest -q tests/test_stage5_*.py` 全部通过。
