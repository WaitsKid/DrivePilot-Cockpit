from __future__ import annotations

import copy
import threading
import time
from typing import Any


class DmsStatusStore:
    """Thread-safe status snapshot shared by the camera worker and FastAPI."""

    def __init__(self) -> None:
        self._condition = threading.Condition()
        self._revision = 0
        self._payload: dict[str, Any] = {
            "backend_version": "5.0",
            "service_running": False,
            "models_ready": False,
            "camera_available": False,
            "last_error": "",
            "started_at_ms": 0,
            "updated_at_ms": int(time.time() * 1000.0),
            "fatigue_level": 1,
            "raw_fatigue_level": 1,
            "status": "normal",
            "status_text": "服务正在初始化",
            "monitoring_state": "initializing",
            "face_detected": False,
            "event_id": 0,
            "event_type": "",
            "message": "",
        }

    def publish(self, payload: dict[str, Any]) -> int:
        with self._condition:
            self._payload = copy.deepcopy(payload)
            self._revision += 1
            self._condition.notify_all()
            return self._revision

    def merge(self, values: dict[str, Any]) -> int:
        with self._condition:
            payload = copy.deepcopy(self._payload)
            payload.update(copy.deepcopy(values))
            payload["updated_at_ms"] = int(time.time() * 1000.0)
            self._payload = payload
            self._revision += 1
            self._condition.notify_all()
            return self._revision

    def snapshot(self) -> tuple[int, dict[str, Any]]:
        with self._condition:
            return self._revision, copy.deepcopy(self._payload)

    def wait_for_change(
        self,
        after_revision: int,
        timeout_seconds: float,
    ) -> tuple[int, dict[str, Any]]:
        with self._condition:
            if self._revision <= after_revision:
                self._condition.wait(timeout=max(0.0, timeout_seconds))
            return self._revision, copy.deepcopy(self._payload)
