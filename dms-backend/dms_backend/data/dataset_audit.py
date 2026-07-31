from __future__ import annotations

import hashlib
import json
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

from PIL import Image, UnidentifiedImageError
from tqdm import tqdm


@dataclass(frozen=True)
class ImageRecord:
    path: str
    split: str
    class_name: str
    width: int
    height: int
    mode: str
    file_size: int
    sha256: str


@dataclass(frozen=True)
class BrokenImage:
    path: str
    error: str


def load_yaml_config(path: Path) -> dict:
    import yaml

    with path.open("r", encoding="utf-8") as file:
        config = yaml.safe_load(file)
    if not isinstance(config, dict):
        raise ValueError(f"配置文件不是有效对象: {path}")
    return config


def sha256_file(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        while chunk := file.read(chunk_size):
            digest.update(chunk)
    return digest.hexdigest()


def iter_image_paths(
    dataset_root: Path,
    expected_layout: dict[str, list[str]],
    allowed_extensions: set[str],
) -> Iterable[tuple[str, str, Path]]:
    for split, class_names in expected_layout.items():
        for class_name in class_names:
            class_dir = dataset_root / split / class_name
            if not class_dir.is_dir():
                raise FileNotFoundError(f"缺少目录: {class_dir}")
            for path in sorted(class_dir.rglob("*")):
                if path.is_file() and path.suffix.lower() in allowed_extensions:
                    yield split, class_name, path


def inspect_image(path: Path, split: str, class_name: str) -> ImageRecord:
    try:
        with Image.open(path) as image:
            image.verify()
        with Image.open(path) as image:
            width, height = image.size
            mode = image.mode
    except (UnidentifiedImageError, OSError, ValueError) as error:
        raise ValueError(str(error)) from error

    return ImageRecord(
        path=str(path.resolve()),
        split=split,
        class_name=class_name,
        width=width,
        height=height,
        mode=mode,
        file_size=path.stat().st_size,
        sha256=sha256_file(path),
    )


def audit_dataset(config: dict) -> tuple[list[ImageRecord], list[BrokenImage], dict]:
    dataset_root = Path(config["dataset_root"]).expanduser().resolve()
    expected_layout = config["expected_layout"]
    allowed_extensions = {value.lower() for value in config["allowed_extensions"]}

    candidates = list(iter_image_paths(dataset_root, expected_layout, allowed_extensions))
    records: list[ImageRecord] = []
    broken: list[BrokenImage] = []

    for split, class_name, path in tqdm(candidates, desc="检查图片", unit="张"):
        try:
            records.append(inspect_image(path, split, class_name))
        except ValueError as error:
            broken.append(BrokenImage(path=str(path.resolve()), error=str(error)))

    class_counts = Counter((record.split, record.class_name) for record in records)
    hash_groups: dict[str, list[ImageRecord]] = defaultdict(list)
    for record in records:
        hash_groups[record.sha256].append(record)

    duplicate_groups = [group for group in hash_groups.values() if len(group) > 1]
    cross_split_duplicates = [
        group for group in duplicate_groups if len({item.split for item in group}) > 1
    ]

    widths = [record.width for record in records]
    heights = [record.height for record in records]
    report = {
        "dataset_root": str(dataset_root),
        "valid_image_count": len(records),
        "broken_image_count": len(broken),
        "class_counts": {
            f"{split}/{class_name}": count
            for (split, class_name), count in sorted(class_counts.items())
        },
        "image_width": {
            "min": min(widths, default=0),
            "max": max(widths, default=0),
        },
        "image_height": {
            "min": min(heights, default=0),
            "max": max(heights, default=0),
        },
        "duplicate_group_count": len(duplicate_groups),
        "cross_split_duplicate_group_count": len(cross_split_duplicates),
        "cross_split_duplicates": [
            [asdict(item) for item in group] for group in cross_split_duplicates[:50]
        ],
    }
    return records, broken, report


def save_audit_outputs(
    output_dir: Path,
    records: list[ImageRecord],
    broken: list[BrokenImage],
    report: dict,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    with (output_dir / "dataset_records.jsonl").open("w", encoding="utf-8") as file:
        for record in records:
            file.write(json.dumps(asdict(record), ensure_ascii=False) + "\n")

    with (output_dir / "broken_images.json").open("w", encoding="utf-8") as file:
        json.dump([asdict(item) for item in broken], file, ensure_ascii=False, indent=2)

    with (output_dir / "dataset_report.json").open("w", encoding="utf-8") as file:
        json.dump(report, file, ensure_ascii=False, indent=2)

    markdown_lines = [
        "# Dataset Audit Report",
        "",
        f"- Dataset root: `{report['dataset_root']}`",
        f"- Valid images: **{report['valid_image_count']}**",
        f"- Broken images: **{report['broken_image_count']}**",
        f"- Duplicate hash groups: **{report['duplicate_group_count']}**",
        f"- Cross-split duplicate groups: **{report['cross_split_duplicate_group_count']}**",
        "",
        "## Class counts",
        "",
        "| Split/Class | Count |",
        "|---|---:|",
    ]
    for name, count in report["class_counts"].items():
        markdown_lines.append(f"| {name} | {count} |")
    markdown_lines.extend(
        [
            "",
            "## Important",
            "",
            "`Closed/Open` are eye-state cues, while `yawn/no_yawn` are mouth-state cues. "
            "They are not the final four DMS fatigue levels.",
        ]
    )
    (output_dir / "dataset_report.md").write_text(
        "\n".join(markdown_lines), encoding="utf-8"
    )
