# DriveGuard DMS Backend

驾驶员疲劳监测训练、评估、ONNX 部署和 FastAPI 服务。

## Pipeline

```text
Camera
  → YuNet face/landmarks
  → eye ROI × 2 → MobileNetV2 Closed/Open
  → face ROI → MobileNetV2 yawn/no_yawn
  → temporal engine
  → energetic / normal / slight / severe
  → REST + WebSocket
```

## Why Two Classifiers

数据集的四个文件夹不是四级疲劳标签：

- `Closed/Open` 是眼睛状态；
- `yawn/no_yawn` 是哈欠状态。

最终疲劳等级由连续闭眼、PERCLOS、哈欠次数和时间防抖融合产生。

## Environment

Python 3.11 is recommended.

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements-stage5.txt
```

## Training Workflow

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

Dataset path is configured in `configs/stage1.yaml`.

## Inference and Service

Place models as described in [the deployment guide](../docs/06-deployment-guide.md).

```powershell
python scripts/11_validate_inference_stack.py
python scripts/13_camera_demo.py
python scripts/15_validate_stage5.py
python scripts/16_run_server.py
```

## Privacy

- no API exposes camera images;
- raw frames are not stored by default;
- datasets, personal data, checkpoints and ONNX files are ignored.

See [MODEL_CARD.md](MODEL_CARD.md).
