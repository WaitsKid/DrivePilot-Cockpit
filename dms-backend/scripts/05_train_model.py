from __future__ import annotations

import argparse
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.training.config import load_training_config
from dms_backend.training.trainer import train_task


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train one MobileNetV2 DMS cue model")
    parser.add_argument(
        "--task",
        required=True,
        choices=["eye_state", "yawn_state"],
        help="训练眼睛状态模型或哈欠状态模型",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=PROJECT_ROOT / "configs" / "training.yaml",
    )
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    config = load_training_config(arguments.config, arguments.task)
    train_task(config)


if __name__ == "__main__":
    main()
