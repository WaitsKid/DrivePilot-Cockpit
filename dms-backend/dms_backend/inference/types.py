from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any

import numpy as np


@dataclass(frozen=True)
class BoundingBox:
    x: int
    y: int
    width: int
    height: int

    @property
    def x2(self) -> int:
        return self.x + self.width

    @property
    def y2(self) -> int:
        return self.y + self.height

    @property
    def area(self) -> int:
        return max(0, self.width) * max(0, self.height)

    def as_tuple(self) -> tuple[int, int, int, int]:
        return self.x, self.y, self.width, self.height


@dataclass(frozen=True)
class FaceDetection:
    box: BoundingBox
    landmarks: np.ndarray
    score: float

    @property
    def right_eye(self) -> tuple[float, float]:
        return float(self.landmarks[0, 0]), float(self.landmarks[0, 1])

    @property
    def left_eye(self) -> tuple[float, float]:
        return float(self.landmarks[1, 0]), float(self.landmarks[1, 1])

    @property
    def nose(self) -> tuple[float, float]:
        return float(self.landmarks[2, 0]), float(self.landmarks[2, 1])

    @property
    def right_mouth(self) -> tuple[float, float]:
        return float(self.landmarks[3, 0]), float(self.landmarks[3, 1])

    @property
    def left_mouth(self) -> tuple[float, float]:
        return float(self.landmarks[4, 0]), float(self.landmarks[4, 1])


@dataclass(frozen=True)
class BinaryPrediction:
    task: str
    class_names: list[str]
    probabilities: list[float]
    predicted_index: int
    predicted_label: str
    risk_class_index: int
    risk_probability: float
    risk_threshold: float
    risk_detected: bool
    inference_ms: float

    def probability_map(self) -> dict[str, float]:
        return {
            label: float(self.probabilities[index])
            for index, label in enumerate(self.class_names)
        }


@dataclass
class DmsFrameResult:
    face_detected: bool
    face_score: float = 0.0
    face_box: BoundingBox | None = None
    right_eye_box: BoundingBox | None = None
    left_eye_box: BoundingBox | None = None
    face_roi_box: BoundingBox | None = None
    right_eye: BinaryPrediction | None = None
    left_eye: BinaryPrediction | None = None
    yawn: BinaryPrediction | None = None
    combined_closed_probability: float = 0.0
    both_eyes_closed: bool = False
    yawn_detected: bool = False
    detection_ms: float = 0.0
    total_ms: float = 0.0
    warnings: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        payload: dict[str, Any] = {
            "face_detected": self.face_detected,
            "face_score": float(self.face_score),
            "combined_closed_probability": float(self.combined_closed_probability),
            "both_eyes_closed": bool(self.both_eyes_closed),
            "yawn_detected": bool(self.yawn_detected),
            "detection_ms": float(self.detection_ms),
            "total_ms": float(self.total_ms),
            "warnings": list(self.warnings),
        }
        for field_name in ("face_box", "right_eye_box", "left_eye_box", "face_roi_box"):
            value = getattr(self, field_name)
            payload[field_name] = asdict(value) if value is not None else None
        for field_name in ("right_eye", "left_eye", "yawn"):
            prediction = getattr(self, field_name)
            if prediction is None:
                payload[field_name] = None
                continue
            prediction_dict = asdict(prediction)
            prediction_dict["probability_map"] = prediction.probability_map()
            payload[field_name] = prediction_dict
        return payload
