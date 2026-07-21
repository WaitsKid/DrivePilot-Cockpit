from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

import argparse
from dms_backend.data.dataset_audit import load_yaml_config
from dms_backend.data.manifest import build_manifests, load_records


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build eye/yawn train-val-test manifests")
    parser.add_argument("--config", type=Path, default=Path("configs/stage1.yaml"))
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config = load_yaml_config(args.config)
    output_dir = Path(config["output_dir"]).resolve()
    records_path = output_dir / "dataset_records.jsonl"
    if not records_path.exists():
        raise SystemExit("请先运行 scripts/02_audit_dataset.py")

    records = load_records(records_path)
    summary = build_manifests(
        records=records,
        output_dir=output_dir / "manifests",
        validation_ratio=float(config["validation_ratio"]),
        random_seed=int(config["random_seed"]),
    )

    print("\n=== Manifests ready ===")
    for task_name, values in summary.items():
        print(task_name, values)
    print("Output:", output_dir / "manifests")


if __name__ == "__main__":
    main()
