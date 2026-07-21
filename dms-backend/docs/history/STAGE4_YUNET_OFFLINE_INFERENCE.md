# Stage 4 - YuNet + MobileNetV2 离线摄像头推理

## 目标

将 Stage 3 导出的两个 MobileNetV2 ONNX 模型接入真实摄像头画面：

1. YuNet 检测驾驶员人脸及 5 个关键点。
2. 根据双眼关键点分别裁剪左右眼 ROI。
3. 使用眼睛模型分别计算左右眼闭合概率。
4. 使用扩展后的人脸 ROI 计算哈欠概率。
5. 在图片、摄像头和本地视频上显示检测框、概率与单帧耗时。

本阶段只产生单帧视觉信号，不直接判定四级疲劳。连续闭眼时间、PERCLOS、哈欠频率和四级状态将在 Stage 5 的时间窗口状态机中实现。

## 为什么哈欠模型使用完整人脸 ROI

当前数据集中的 `Closed/Open` 通常是眼睛局部图片，而 `yawn/no_yawn` 通常是完整人脸图片。部署时必须尽量保持与训练数据相似：

- 眼睛模型：左右眼局部 ROI。
- 哈欠模型：扩展后的人脸 ROI。

若把哈欠模型突然改成只有嘴巴的局部裁剪，会产生明显训练域与部署域不一致。

## 1. 覆盖 Stage 4 代码

优先使用增量包覆盖现有项目，保留本机的：

- `artifacts/training/`
- `artifacts/evaluation/`
- `artifacts/onnx/`

## 2. 安装依赖

```bash
pip install -r requirements-stage4.txt
```

## 3. 放置 YuNet 模型

将模型放到：

```text
models/yunet/face_detection_yunet_2026may.onnx
```

若动态输入版本在当前 OpenCV 环境中报错，再下载固定输入版本，并把 `configs/stage4.yaml` 改成：

```yaml
models:
  yunet: "models/yunet/face_detection_yunet_2023mar.onnx"
```

## 4. 验证完整推理环境

```bash
python scripts/11_validate_inference_stack.py
```

预期：

```text
YuNet initialization: PASS
MobileNetV2 ONNX smoke tests: PASS
Inference stack validation: PASS
```

## 5. 单张图片测试

```bash
python scripts/12_infer_image.py --image "D:/test/normal.jpg" --show
```

结果保存到：

```text
artifacts/stage4/images/
```

## 6. 摄像头实时测试

```bash
python scripts/13_camera_demo.py
```

快捷键：

- `Q` 或 `Esc`：退出。
- `S`：保存当前标注图和 JSON。

重点测试：

1. 正常睁眼。
2. 自然眨眼。
3. 持续闭眼 2~3 秒。
4. 正常说话。
5. 明显打哈欠。
6. 轻微侧脸。
7. 戴眼镜和弱光。

## 7. 四段自拍视频离线分析

```bash
python scripts/14_infer_video.py --video "D:/videos/energetic.mp4"
python scripts/14_infer_video.py --video "D:/videos/normal.mp4"
python scripts/14_infer_video.py --video "D:/videos/slight_fatigue.mp4"
python scripts/14_infer_video.py --video "D:/videos/severe_fatigue.mp4"
```

输出：

```text
artifacts/stage4/videos/
├── *_annotated.mp4
└── *_timeline.csv
```

CSV 是 Stage 5 设计时间阈值的重要依据。

## 当前输出含义

- `combined_closed_probability`：左右眼闭合概率平均值。
- `both_eyes_closed`：左右眼都超过各自部署阈值。
- `yawn_detected`：哈欠概率超过验证集选出的部署阈值。
- `total_ms`：YuNet、ROI、两个眼睛推理和哈欠推理的单帧总时间。

## 当前模型阈值

程序不会写死 0.5，而是读取 Stage 3 输出：

```text
artifacts/evaluation/eye_state/deployment_config.json
artifacts/evaluation/yawn_state/deployment_config.json
```

当前结果中：

- Closed 风险阈值：0.43。
- yawn 风险阈值：0.62。

Stage 4 会同时显示连续概率，后续可以根据你自己的摄像头数据再校准。


## 摄像头校准时临时覆盖阈值

默认使用 Stage 3 的 `deployment_config.json`。若需要对比 0.5 或其它阈值，可在 `configs/stage4.yaml` 中设置：

```yaml
eye_state:
  risk_threshold_override: 0.50
yawn_state:
  risk_threshold_override: 0.62
```

保留 `null` 表示继续使用验证集选择的部署阈值。
