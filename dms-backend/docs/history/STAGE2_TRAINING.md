# Stage 2：训练两个 MobileNetV2 视觉线索模型

## 目标

分别训练：

1. `eye_state`：Closed / Open
2. `yawn_state`：no_yawn / yawn

它们仍然不是最终四级疲劳结果。最终 DMS 等级在后面的时间窗口融合阶段产生。

## 运行前检查

Stage 1 必须已生成：

```text
artifacts/manifests/
├── eye_state_train.csv
├── eye_state_val.csv
├── eye_state_test.csv
├── yawn_state_train.csv
├── yawn_state_val.csv
└── yawn_state_test.csv
```

## 训练单个任务

先训练眼睛状态模型：

```bash
python scripts/05_train_model.py --task eye_state
```

再训练哈欠状态模型：

```bash
python scripts/05_train_model.py --task yawn_state
```

也可以依次训练两个模型：

```bash
python scripts/06_train_all_models.py
```

## 训练流程

- 使用 ImageNet 预训练的 MobileNetV2
- 前 3 个 epoch 冻结特征提取骨干
- 之后解冻整个网络微调
- AdamW 优化器，分类头与骨干网络使用不同学习率
- 类别权重处理类别不平衡
- Label smoothing
- CUDA 下启用 AMP
- 梯度裁剪
- ReduceLROnPlateau
- 以验证集 macro F1 保存最佳模型
- Early stopping

## 产物

每个任务都会生成：

```text
artifacts/training/<task>/
├── best.pt
├── last.pt
├── best_metrics.json
├── history.json
├── training_summary.json
├── training_curves.png
└── validation_confusion_matrix.png
```

## 单图快速验证

```bash
python scripts/07_predict_image.py \
  --checkpoint artifacts/training/eye_state/best.pt \
  --image "D:/path/to/one_eye_image.jpg"
```

Windows PowerShell 可以将命令写在一行。

## 首次建议

首次先保持：

```yaml
num_workers: 0
batch_size: 32
```

显存不足时把 `batch_size` 改成 16 或 8。CPU 训练可正常运行，但耗时会明显更长。
