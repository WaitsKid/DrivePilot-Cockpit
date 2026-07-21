# DriveGuard DMS Model Card

## Model Summary

Two independent MobileNetV2 binary classifiers:

| Model | Classes | Input | Deployment |
|---|---|---|---|
| Eye state | Closed, Open | 224×224 RGB eye ROI | ONNX Runtime |
| Yawn state | no_yawn, yawn | 224×224 RGB face ROI | ONNX Runtime |

YuNet is used for face detection and landmarks. A temporal engine converts frame-level probabilities into four fatigue levels.

## Intended Use

- educational HMI demonstration;
- local webcam prototype;
- transfer learning and ONNX deployment exercise;
- backend API integration testing.

## Out-of-Scope Use

- real-world driving safety decisions;
- medical diagnosis;
- employee surveillance;
- identity recognition;
- production automotive systems.

## Training Data

The project used the Kaggle `yawn-eye-dataset-new` structure. The dataset itself is not distributed. Public training data may contain demographic, lighting, device and capture biases.

## Evaluation

| Metric | Eye | Yawn |
|---|---:|---:|
| Test Macro F1 (argmax) | 0.9908 | 0.9814 |
| Risk threshold | Closed 0.43 | yawn 0.62 |
| Threshold Macro F1 | 0.9862 | 0.9907 |
| Misclassified test samples | 2/218 | 4/215 |
| ONNX CPU P95 | 3.120 ms | 4.032 ms |

## Temporal Rules

Default examples:

- slight: closed 1.10 s, PERCLOS 0.30, or 2 yawns;
- severe: closed 2.50 s, PERCLOS 0.52, or 3 yawns;
- recovery is slower than escalation;
- missing face does not accumulate fatigue.

## Limitations

- strong dataset-domain bias is possible;
- glasses, glare, masks, side pose, low light and camera placement may degrade performance;
- yawn may be confused with talking or laughing;
- thresholds are heuristic and require calibration;
- no head-pose or gaze estimator is included;
- metrics do not establish safety fitness.

## Privacy

Frames stay in the local DMS process. The repository excludes personal images and recordings.
