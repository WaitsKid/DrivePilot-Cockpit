from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml

from dms_backend.inference.config import project_root, resolve_project_path


@dataclass(frozen=True)
class FatigueConfig:
    perclos_window_seconds: float = 30.0
    yawn_window_seconds: float = 60.0
    minimum_valid_window_seconds: float = 8.0
    maximum_sample_gap_seconds: float = 0.50
    eye_open_release_seconds: float = 0.22
    minimum_yawn_event_seconds: float = 0.80
    yawn_release_seconds: float = 0.45
    slight_closed_seconds: float = 1.10
    severe_closed_seconds: float = 2.50
    slight_perclos: float = 0.30
    severe_perclos: float = 0.52
    slight_yawn_count: int = 2
    severe_yawn_count: int = 3
    energetic_perclos_max: float = 0.10
    energetic_minimum_tracking_seconds: float = 18.0
    slight_escalation_hold_seconds: float = 0.60
    severe_escalation_hold_seconds: float = 0.30
    recovery_from_severe_seconds: float = 10.0
    recovery_from_slight_seconds: float = 8.0
    recovery_to_energetic_seconds: float = 15.0
    face_missing_reset_seconds: float = 12.0
    slight_reminder_cooldown_seconds: float = 45.0
    severe_reminder_cooldown_seconds: float = 20.0


@dataclass(frozen=True)
class ServerConfig:
    host: str = "127.0.0.1"
    port: int = 8765
    log_level: str = "info"
    auto_start_monitoring: bool = True
    websocket_push_interval_ms: int = 250


@dataclass(frozen=True)
class MonitorConfig:
    target_process_fps: float = 8.0
    camera_open_retry_seconds: float = 2.0
    camera_read_failure_limit: int = 15
    mirror_input: bool = True
    publish_every_n_frames: int = 1


@dataclass(frozen=True)
class PrivacyConfig:
    store_camera_frames: bool = False
    expose_camera_frames_over_api: bool = False


@dataclass(frozen=True)
class Stage5Config:
    path: Path
    stage4_config_path: Path
    server: ServerConfig
    monitor: MonitorConfig
    fatigue: FatigueConfig
    privacy: PrivacyConfig


def load_stage5_config(path: str | Path | None = None) -> Stage5Config:
    config_path = Path(path) if path is not None else project_root() / "configs" / "stage5.yaml"
    if not config_path.is_absolute():
        config_path = project_root() / config_path
    if not config_path.is_file():
        raise FileNotFoundError(f"找不到 Stage 5 配置文件: {config_path}")

    with config_path.open("r", encoding="utf-8") as file:
        raw = yaml.safe_load(file)
    if not isinstance(raw, dict):
        raise ValueError(f"Stage 5 配置不是有效对象: {config_path}")

    server_raw = _mapping(raw.get("server"))
    monitor_raw = _mapping(raw.get("monitor"))
    fatigue_raw = _mapping(raw.get("fatigue"))
    alerts_raw = _mapping(raw.get("alerts"))
    privacy_raw = _mapping(raw.get("privacy"))

    fatigue_values: dict[str, Any] = dict(fatigue_raw)
    fatigue_values["slight_reminder_cooldown_seconds"] = alerts_raw.get(
        "slight_reminder_cooldown_seconds", 45.0
    )
    fatigue_values["severe_reminder_cooldown_seconds"] = alerts_raw.get(
        "severe_reminder_cooldown_seconds", 20.0
    )

    stage4_config_value = raw.get("stage4_config", "configs/stage4.yaml")
    return Stage5Config(
        path=config_path.resolve(),
        stage4_config_path=resolve_project_path(stage4_config_value).resolve(),
        server=ServerConfig(**_coerce_dataclass_values(ServerConfig, server_raw)),
        monitor=MonitorConfig(**_coerce_dataclass_values(MonitorConfig, monitor_raw)),
        fatigue=FatigueConfig(**_coerce_dataclass_values(FatigueConfig, fatigue_values)),
        privacy=PrivacyConfig(**_coerce_dataclass_values(PrivacyConfig, privacy_raw)),
    )


def _mapping(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def _coerce_dataclass_values(dataclass_type: type, raw: dict[str, Any]) -> dict[str, Any]:
    allowed = dataclass_type.__dataclass_fields__.keys()
    return {key: value for key, value in raw.items() if key in allowed}
