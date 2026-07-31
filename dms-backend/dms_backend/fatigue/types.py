from __future__ import annotations

from dataclasses import asdict, dataclass, field
from enum import IntEnum
from typing import Any


class FatigueLevel(IntEnum):
    ENERGETIC = 0
    NORMAL = 1
    SLIGHT = 2
    SEVERE = 3

    @property
    def code(self) -> str:
        return {
            FatigueLevel.ENERGETIC: "energetic",
            FatigueLevel.NORMAL: "normal",
            FatigueLevel.SLIGHT: "slight_fatigue",
            FatigueLevel.SEVERE: "severe_fatigue",
        }[self]

    @property
    def text(self) -> str:
        return {
            FatigueLevel.ENERGETIC: "活力充沛",
            FatigueLevel.NORMAL: "状态正常",
            FatigueLevel.SLIGHT: "略微疲劳",
            FatigueLevel.SEVERE: "严重疲劳",
        }[self]


@dataclass(frozen=True)
class VisualCueSample:
    timestamp: float
    face_detected: bool
    eyes_closed: bool
    closed_probability: float
    yawn_detected: bool
    yawn_probability: float
    face_score: float = 0.0
    inference_ms: float = 0.0


@dataclass(frozen=True)
class FatigueEvent:
    event_id: int
    timestamp: float
    level: FatigueLevel
    event_type: str
    message: str
    reason: str

    def to_dict(self) -> dict[str, Any]:
        payload = asdict(self)
        payload["level"] = int(self.level)
        payload["status"] = self.level.code
        payload["status_text"] = self.level.text
        return payload


@dataclass
class FatigueSnapshot:
    timestamp: float
    fatigue_level: FatigueLevel = FatigueLevel.NORMAL
    raw_fatigue_level: FatigueLevel = FatigueLevel.NORMAL
    status_text: str = "状态正常"
    monitoring_state: str = "starting"
    face_detected: bool = False
    face_score: float = 0.0
    closed_probability: float = 0.0
    yawn_probability: float = 0.0
    eyes_closed: bool = False
    yawn_detected: bool = False
    closed_duration_seconds: float = 0.0
    perclos: float = 0.0
    valid_window_seconds: float = 0.0
    yawn_count_window: int = 0
    risk_score: float = 0.0
    inference_ms: float = 0.0
    processed_fps: float = 0.0
    reasons: list[str] = field(default_factory=list)
    event_id: int = 0
    event_type: str = ""
    message: str = ""
    last_event_timestamp: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "timestamp_ms": int(self.timestamp * 1000.0),
            "fatigue_level": int(self.fatigue_level),
            "raw_fatigue_level": int(self.raw_fatigue_level),
            "status": self.fatigue_level.code,
            "status_text": self.status_text,
            "monitoring_state": self.monitoring_state,
            "face_detected": bool(self.face_detected),
            "face_score": round(float(self.face_score), 6),
            "closed_probability": round(float(self.closed_probability), 6),
            "yawn_probability": round(float(self.yawn_probability), 6),
            "eyes_closed": bool(self.eyes_closed),
            "yawn_detected": bool(self.yawn_detected),
            "closed_duration_ms": int(self.closed_duration_seconds * 1000.0),
            "perclos": round(float(self.perclos), 6),
            "valid_window_seconds": round(float(self.valid_window_seconds), 3),
            "yawn_count_window": int(self.yawn_count_window),
            "risk_score": round(float(self.risk_score), 6),
            "inference_ms": round(float(self.inference_ms), 3),
            "processed_fps": round(float(self.processed_fps), 3),
            "reasons": list(self.reasons),
            "event_id": int(self.event_id),
            "event_type": self.event_type,
            "message": self.message,
            "last_event_timestamp_ms": int(self.last_event_timestamp * 1000.0),
        }
