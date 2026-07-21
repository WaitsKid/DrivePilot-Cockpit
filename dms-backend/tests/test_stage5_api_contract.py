from __future__ import annotations

from fastapi.testclient import TestClient

from dms_backend.api.app import create_app
from dms_backend.fatigue.config import load_stage5_config
from dms_backend.service.status_store import DmsStatusStore


class FakeService:
    def __init__(self) -> None:
        self.store = DmsStatusStore()
        self.models_ready = True
        self.store.merge(
            {
                "models_ready": True,
                "service_running": True,
                "camera_available": True,
                "monitoring_state": "tracking",
                "status_text": "状态正常",
                "last_error": "",
            }
        )

    def initialize_models(self) -> bool:
        return True

    def start(self) -> bool:
        self.store.merge({"service_running": True})
        return True

    def stop(self, join_timeout_seconds: float = 5.0) -> None:
        del join_timeout_seconds
        self.store.merge({"service_running": False, "camera_available": False})

    def shutdown(self) -> None:
        self.stop()

    def get_status(self):
        return self.store.snapshot()[1]

    def get_public_config(self):
        return {"privacy": {"expose_camera_frames_over_api": False}}

    def reset_state(self):
        self.store.merge({"fatigue_level": 1, "event_id": 0})
        return self.get_status()


def test_rest_contract_without_models_or_camera() -> None:
    fake = FakeService()
    app = create_app("configs/stage5.yaml", service=fake)  # type: ignore[arg-type]
    with TestClient(app) as client:
        health = client.get("/health")
        assert health.status_code == 200
        assert health.json()["models_ready"] is True

        status = client.get("/api/v1/dms/status")
        assert status.status_code == 200
        assert "fatigue_level" in status.json()

        public_config = client.get("/api/v1/dms/config")
        assert public_config.json()["privacy"]["expose_camera_frames_over_api"] is False

        stopped = client.post("/api/v1/dms/stop")
        assert stopped.json()["service_running"] is False
