from __future__ import annotations

from copy import deepcopy
from pathlib import Path
from typing import Any

import yaml

from dms_backend.common.labels import TASKS


def load_training_config(path: Path, task_name: str) -> dict[str, Any]:
    """Load Stage 2 YAML and merge global settings with one task section."""
    with path.open("r", encoding="utf-8") as file:
        raw = yaml.safe_load(file)

    if not isinstance(raw, dict):
        raise ValueError(f"训练配置不是有效对象: {path}")
    if task_name not in TASKS:
        raise ValueError(f"未知任务 {task_name!r}，可选值: {sorted(TASKS)}")

    task_settings = raw.get("tasks", {}).get(task_name)
    if not isinstance(task_settings, dict):
        raise ValueError(f"配置中缺少 tasks.{task_name}")

    config = deepcopy(raw)
    config["task_name"] = task_name
    config["task"] = deepcopy(task_settings)
    config.pop("tasks", None)

    manifest_dir = Path(config["manifest_dir"]).expanduser()
    output_root = Path(config["output_root"]).expanduser()
    config["manifest_dir"] = str(manifest_dir)
    config["output_root"] = str(output_root)
    return config
