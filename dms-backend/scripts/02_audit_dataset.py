from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

import argparse
from dms_backend.data.dataset_audit import (
    audit_dataset,
    load_yaml_config,
    save_audit_outputs,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit the Kaggle DMS dataset")
    parser.add_argument(
        "--config",
        type=Path,
        default=Path("configs/stage1.yaml"),
        help="Stage 1 YAML configuration",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config = load_yaml_config(args.config)
    output_dir = Path(config["output_dir"]).resolve()

    records, broken, report = audit_dataset(config)
    save_audit_outputs(output_dir, records, broken, report)

    print("\n=== Dataset audit complete ===")
    print("Valid images:", report["valid_image_count"])
    print("Broken images:", report["broken_image_count"])
    print("Cross-split duplicate groups:", report["cross_split_duplicate_group_count"])
    print("Report:", output_dir / "dataset_report.md")


if __name__ == "__main__":
    main()
