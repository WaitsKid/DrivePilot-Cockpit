from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.training.config import load_training_config
from dms_backend.training.trainer import train_task


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train eye and yawn MobileNetV2 models")
    parser.add_argument(
        "--config",
        type=Path,
        default=PROJECT_ROOT / "configs" / "training.yaml",
    )
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    summaries = []
    for task_name in ("eye_state", "yawn_state"):
        config = load_training_config(arguments.config, task_name)
        summaries.append(train_task(config))

    output_path = PROJECT_ROOT / "artifacts" / "training" / "training_summary.json"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(summaries, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(f"\ntraining complete: {output_path}")


if __name__ == "__main__":
    main()
