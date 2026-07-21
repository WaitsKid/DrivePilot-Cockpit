from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from dms_backend.training.metrics import compute_classification_metrics


@dataclass(frozen=True)
class PredictionRecord:
    path: str
    target: int
    prediction: int
    confidence: float
    probabilities: list[float]


def _predictions_from_risk_threshold(
    records: list[PredictionRecord],
    *,
    risk_class_index: int,
    threshold: float,
) -> list[int]:
    safe_class_index = 1 - risk_class_index
    return [
        risk_class_index
        if record.probabilities[risk_class_index] >= threshold
        else safe_class_index
        for record in records
    ]


def find_best_risk_threshold(
    records: list[PredictionRecord],
    *,
    class_names: dict[int, str],
    risk_class_index: int,
    minimum: float,
    maximum: float,
    step: float,
) -> tuple[float, dict[str, Any]]:
    if not records:
        raise ValueError("验证集为空，无法选择阈值")

    targets = [record.target for record in records]
    best_threshold = 0.5
    best_metrics: dict[str, Any] | None = None
    best_score = -1.0
    best_risk_recall = -1.0

    threshold = minimum
    while threshold <= maximum + 1e-9:
        predictions = _predictions_from_risk_threshold(
            records,
            risk_class_index=risk_class_index,
            threshold=threshold,
        )
        metrics = compute_classification_metrics(targets, predictions, class_names).to_dict()
        risk_name = class_names[risk_class_index]
        score = float(metrics["macro_f1"])
        risk_recall = float(metrics["per_class"][risk_name]["recall"])

        if score > best_score + 1e-12 or (
            abs(score - best_score) <= 1e-12 and risk_recall > best_risk_recall
        ):
            best_threshold = round(float(threshold), 6)
            best_metrics = metrics
            best_score = score
            best_risk_recall = risk_recall
        threshold += step

    assert best_metrics is not None
    return best_threshold, best_metrics
