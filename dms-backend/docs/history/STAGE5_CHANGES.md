# Stage 5 改动清单

## 新增配置

- `configs/stage5.yaml`

## 新增疲劳状态模块

- `dms_backend/fatigue/config.py`
- `dms_backend/fatigue/types.py`
- `dms_backend/fatigue/engine.py`

功能包括：

- 持续闭眼时间；
- 时间加权 PERCLOS；
- 哈欠事件去重与滑动窗口计数；
- 活力、正常、略微疲劳、严重疲劳四级状态；
- 状态升级与恢复防抖；
- 告警冷却与 `event_id` 去重；
- 长时间无人脸后的时间窗复位。

## 新增后台服务

- `dms_backend/service/status_store.py`
- `dms_backend/service/background_service.py`

功能包括：

- Python 服务独占摄像头；
- 无预览窗口后台推理；
- 摄像头断开自动重连；
- 线程安全状态快照；
- 不保存、不传输摄像头画面。

## 新增 FastAPI

- `dms_backend/api/app.py`

接口：

- `GET /health`
- `GET /api/v1/dms/status`
- `GET /api/v1/dms/config`
- `POST /api/v1/dms/start`
- `POST /api/v1/dms/stop`
- `POST /api/v1/dms/reset`
- `WS /api/v1/dms/events`

## 新增脚本

- `scripts/15_validate_stage5.py`
- `scripts/16_run_server.py`
- `scripts/17_check_service.py`
- `scripts/18_simulate_state_machine.py`

## 新增测试

- `tests/test_stage5_fatigue_engine.py`
- `tests/test_stage5_status_store.py`
- `tests/test_stage5_api_contract.py`

## 修改文档

- `README.md`
- `STAGE5_FASTAPI_FATIGUE_SERVICE.md`
- `requirements-stage5.txt`
