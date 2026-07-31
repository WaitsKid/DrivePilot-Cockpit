# DriveGuard DMS Backend

这是 DrivePilot Cockpit 的驾驶员疲劳监测服务，包含数据整理、模型训练、评估、ONNX 导出、摄像头推理和 FastAPI 接口。

## 推理流程

```text
Camera
  → YuNet face / landmarks
  → left eye + right eye ROI → MobileNetV2 Closed / Open
  → face ROI → MobileNetV2 yawn / no_yawn
  → temporal fatigue engine
  → energetic / normal / slight / severe
  → REST + WebSocket
```

数据集中的 `Closed/Open` 和 `yawn/no_yawn` 是两个视觉分类任务，不是四级疲劳标签。最终状态由连续闭眼时间、PERCLOS、哈欠次数和状态防抖共同计算。

## 运行环境

推荐 Python 3.11。

运行服务只需要：

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements-runtime.txt
```

需要训练和导出模型时安装：

```powershell
pip install -r requirements-training.txt
```

## 配置文件

| 文件 | 用途 |
|---|---|
| `configs/dataset.yaml` | 数据集路径、类别和清单输出 |
| `configs/training.yaml` | MobileNetV2 训练参数 |
| `configs/evaluation.yaml` | 测试集评估、阈值和 ONNX 导出 |
| `configs/inference.yaml` | YuNet、ROI、ONNX 模型与摄像头参数 |
| `configs/service.yaml` | FastAPI、疲劳状态机和隐私设置 |

## 训练与导出

```powershell
python scripts/01_check_environment.py
python scripts/02_audit_dataset.py
python scripts/03_build_manifests.py
python scripts/05_train_model.py --task eye_state
python scripts/05_train_model.py --task yawn_state
python scripts/08_evaluate_test_set.py --task all
python scripts/09_export_onnx.py --task all
python scripts/10_benchmark_onnx.py --task all
```

## 摄像头与服务

```powershell
python scripts/11_validate_inference_stack.py
python scripts/13_camera_preview.py
python scripts/15_validate_runtime.py
python scripts/16_run_server.py
```

默认服务地址：

```text
http://127.0.0.1:8765
ws://127.0.0.1:8765/api/v1/dms/events
```

## 隐私

- API 不提供摄像头图片接口；
- 默认不保存原始帧；
- 数据集、个人采集数据、训练权重和 ONNX 文件不提交；
- Qt 只接收疲劳状态、概率、性能指标和告警事件。

模型适用范围与指标说明见 [MODEL_CARD.md](MODEL_CARD.md)。
