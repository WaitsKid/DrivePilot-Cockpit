# Stage 3：独立测试集评估与 ONNX 导出

## 目标
1. 在从未参与训练和阈值选择的 test 清单上正式评估两个模型。
2. 输出每类 precision / recall / F1、混淆矩阵和误判样本。
3. 使用 validation 清单选择风险概率阈值，不使用 test 集调参。
4. 导出 ONNX，并使用 ONNX Runtime 检查输出一致性。
5. 在 CPU 上测量单张推理耗时，为 FastAPI 部署做准备。

## 运行顺序

```bash
pip install -r requirements-stage3.txt
python scripts/08_evaluate_test_set.py --task all
python scripts/09_export_onnx.py --task all
python scripts/10_benchmark_onnx.py --task all
pytest -q
```

## 评估产物

```text
artifacts/evaluation/eye_state/
artifacts/evaluation/yawn_state/
```

每个目录包含：
- `test_report.json`
- `deployment_config.json`
- `test_predictions.csv`
- `test_confusion_matrix_argmax.png`
- `test_confusion_matrix_threshold.png`
- `misclassified_grid.png`
- `misclassified_samples/`

## ONNX 产物

```text
artifacts/onnx/
├── eye_state_mobilenetv2.onnx
├── eye_state_metadata.json
├── eye_state_benchmark.json
├── yawn_state_mobilenetv2.onnx
├── yawn_state_metadata.json
└── yawn_state_benchmark.json
```

## 风险类别
- `eye_state`：Closed 是风险类别，索引为 0。
- `yawn_state`：yawn 是风险类别，索引为 1。

后续 FastAPI 不只使用最终标签，还会读取 Closed / yawn 的概率并输入时间窗口状态机。
