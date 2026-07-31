from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml


def project_root() -> Path:
    return Path(__file__).resolve().parents[2]


def load_inference_config(path: str | Path | None = None) -> dict[str, Any]:
    config_path = Path(path) if path is not None else project_root() / "configs" / "inference.yaml"
    if not config_path.is_absolute():
        config_path = project_root() / config_path
    if not config_path.is_file():
        raise FileNotFoundError(f"找不到推理配置文件: {config_path}")
    with config_path.open("r", encoding="utf-8") as file:
        raw = yaml.safe_load(file)
    if not isinstance(raw, dict):
        raise ValueError(f"推理配置不是有效对象: {config_path}")
    raw["_config_path"] = str(config_path.resolve())
    return raw


def resolve_project_path(value: str | Path) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return project_root() / path
