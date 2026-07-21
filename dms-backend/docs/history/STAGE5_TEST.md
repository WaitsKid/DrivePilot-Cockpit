# Stage 5 测试项

## 安装

```bash
pip install -r requirements-stage5.txt
```

## 静态验证

```bash
python scripts/15_validate_stage5.py
pytest -q tests/test_stage5_*.py
```

## 状态机模拟

```bash
python scripts/18_simulate_state_machine.py
```

## 启动服务

先关闭 Stage 4 摄像头演示窗口，确保摄像头没有被其它进程占用：

```bash
python scripts/16_run_server.py
```

## 服务检查

另开 Terminal：

```bash
python scripts/17_check_service.py --watch-seconds 30
```

## 人工动作测试

1. 正常注视 20 秒；
2. 持续闭眼约 1.5 秒，观察是否进入略微疲劳；
3. 正常睁眼约 8～10 秒，观察是否逐步恢复；
4. 持续闭眼约 3 秒，观察是否进入严重疲劳；
5. 模拟两次持续哈欠，观察 60 秒哈欠计数；
6. 离开画面，观察 `face_detected=false`；
7. 连续离开约 12 秒，观察时间窗复位；
8. 调用 `/api/v1/dms/stop` 后确认摄像头被释放；
9. 再调用 `/api/v1/dms/start`，确认可以重新打开摄像头；
10. 检查相同 `event_id` 不会被当成新告警。

## 隐私检查

确认：

- 没有摄像头预览窗口；
- API JSON 中没有图片或 Base64；
- 项目没有自动生成截图或录像；
- `GET /api/v1/dms/config` 中两个隐私字段都为 `false`。
