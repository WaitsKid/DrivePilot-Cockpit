from __future__ import annotations

import argparse
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.deployment.onnx_exporter import benchmark_onnx_model


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Benchmark ONNX models with ONNX Runtime CPU")
    parser.add_argument("--task", choices=["eye_state", "yawn_state", "all"], default="all")
    parser.add_argument("--config", type=Path, default=PROJECT_ROOT / "configs" / "stage3.yaml")
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    tasks = ["eye_state", "yawn_state"] if arguments.task == "all" else [arguments.task]
    for task_name in tasks:
        benchmark_onnx_model(stage3_config_path=arguments.config, task_name=task_name)


if __name__ == "__main__":
    main()
