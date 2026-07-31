from __future__ import annotations

from dataclasses import asdict, dataclass

import numpy as np


@dataclass(frozen=True)
class ClassificationMetrics:
    accuracy: float
    macro_precision: float
    macro_recall: float
    macro_f1: float
    per_class: dict[str, dict[str, float | int]]
    confusion_matrix: list[list[int]]

    def to_dict(self) -> dict:
        return asdict(self)


def compute_classification_metrics(
    targets: list[int],
    predictions: list[int],
    class_names: dict[int, str],
) -> ClassificationMetrics:
    if len(targets) != len(predictions):
        raise ValueError("targets 与 predictions 数量不一致")
    if not targets:
        raise ValueError("没有样本，无法计算指标")

    num_classes = len(class_names)
    confusion = np.zeros((num_classes, num_classes), dtype=np.int64)
    for target, prediction in zip(targets, predictions, strict=True):
        confusion[target, prediction] += 1

    per_class: dict[str, dict[str, float | int]] = {}
    precisions: list[float] = []
    recalls: list[float] = []
    f1_scores: list[float] = []

    for index in range(num_classes):
        true_positive = int(confusion[index, index])
        false_positive = int(confusion[:, index].sum() - true_positive)
        false_negative = int(confusion[index, :].sum() - true_positive)
        support = int(confusion[index, :].sum())

        precision = true_positive / max(1, true_positive + false_positive)
        recall = true_positive / max(1, true_positive + false_negative)
        f1 = 2 * precision * recall / max(1e-12, precision + recall)

        precisions.append(precision)
        recalls.append(recall)
        f1_scores.append(f1)
        per_class[class_names[index]] = {
            "precision": precision,
            "recall": recall,
            "f1": f1,
            "support": support,
        }

    accuracy = float(np.trace(confusion) / max(1, confusion.sum()))
    return ClassificationMetrics(
        accuracy=accuracy,
        macro_precision=float(np.mean(precisions)),
        macro_recall=float(np.mean(recalls)),
        macro_f1=float(np.mean(f1_scores)),
        per_class=per_class,
        confusion_matrix=confusion.tolist(),
    )
