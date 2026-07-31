from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

import argparse
import csv
import random

import matplotlib.pyplot as plt
from PIL import Image


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Preview samples from one manifest")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("artifacts/manifests/eye_state_train.csv"),
    )
    parser.add_argument("--count", type=int, default=12)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    with args.manifest.open("r", encoding="utf-8-sig", newline="") as file:
        rows = list(csv.DictReader(file))
    if not rows:
        raise SystemExit(f"清单为空: {args.manifest}")

    sample_count = min(args.count, len(rows))
    samples = random.Random(20260720).sample(rows, sample_count)
    columns = 4
    rows_count = (sample_count + columns - 1) // columns
    figure, axes = plt.subplots(rows_count, columns, figsize=(12, rows_count * 3))
    axes = axes.flatten() if hasattr(axes, "flatten") else [axes]

    for axis, item in zip(axes, samples):
        with Image.open(item["path"]) as image:
            axis.imshow(image.convert("RGB"))
        axis.set_title(f"{item['class_name']} | {Path(item['path']).name}")
        axis.axis("off")

    for axis in axes[len(samples):]:
        axis.axis("off")

    figure.tight_layout()
    output_path = Path("artifacts") / f"preview_{args.manifest.stem}.png"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output_path, dpi=150)
    print("Preview saved:", output_path.resolve())
    plt.show()


if __name__ == "__main__":
    main()
