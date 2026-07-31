from __future__ import annotations

import csv
import json
import random
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from dms_backend.common.labels import TASKS, BinaryTask
from dms_backend.data.dataset_audit import ImageRecord


@dataclass(frozen=True)
class ManifestRow:
    path: str
    label: int
    class_name: str
    task: str
    split: str
    sha256: str


def load_records(path: Path) -> list[ImageRecord]:
    records: list[ImageRecord] = []
    with path.open("r", encoding="utf-8") as file:
        for line in file:
            payload = json.loads(line)
            records.append(ImageRecord(**payload))
    return records


def _group_records_by_hash(records: list[ImageRecord]) -> list[list[ImageRecord]]:
    grouped: dict[str, list[ImageRecord]] = defaultdict(list)
    for record in records:
        grouped[record.sha256].append(record)
    return list(grouped.values())


def _split_train_and_validation(
    records: list[ImageRecord],
    validation_ratio: float,
    random_seed: int,
) -> tuple[list[ImageRecord], list[ImageRecord]]:
    train_records: list[ImageRecord] = []
    validation_records: list[ImageRecord] = []
    rng = random.Random(random_seed)

    by_class: dict[str, list[ImageRecord]] = defaultdict(list)
    for record in records:
        by_class[record.class_name].append(record)

    for class_name, class_records in sorted(by_class.items()):
        hash_groups = _group_records_by_hash(class_records)
        rng.shuffle(hash_groups)
        validation_target = max(1, round(len(class_records) * validation_ratio))
        current_validation_count = 0

        for group in hash_groups:
            if current_validation_count < validation_target:
                validation_records.extend(group)
                current_validation_count += len(group)
            else:
                train_records.extend(group)

        if not train_records:
            raise ValueError(f"类别 {class_name} 没有留下训练样本")

    return train_records, validation_records


def _exclude_test_leakage(
    train_records: list[ImageRecord],
    test_records: list[ImageRecord],
) -> tuple[list[ImageRecord], list[ImageRecord]]:
    train_hashes = {record.sha256 for record in train_records}
    clean_test = [record for record in test_records if record.sha256 not in train_hashes]
    removed_test = [record for record in test_records if record.sha256 in train_hashes]
    return clean_test, removed_test


def _to_rows(records: list[ImageRecord], task: BinaryTask, split: str) -> list[ManifestRow]:
    rows: list[ManifestRow] = []
    for record in records:
        if record.class_name not in task.class_to_index:
            continue
        rows.append(
            ManifestRow(
                path=record.path,
                label=task.class_to_index[record.class_name],
                class_name=record.class_name,
                task=task.name,
                split=split,
                sha256=record.sha256,
            )
        )
    return rows


def write_manifest(path: Path, rows: list[ManifestRow]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.DictWriter(
            file,
            fieldnames=["path", "label", "class_name", "task", "split", "sha256"],
        )
        writer.writeheader()
        for row in rows:
            writer.writerow(row.__dict__)


def build_manifests(
    records: list[ImageRecord],
    output_dir: Path,
    validation_ratio: float,
    random_seed: int,
) -> dict:
    summary: dict[str, dict] = {}

    for task_name, task in TASKS.items():
        task_train_all = [
            record
            for record in records
            if record.split == "train" and record.class_name in task.class_to_index
        ]
        task_test_all = [
            record
            for record in records
            if record.split == "test" and record.class_name in task.class_to_index
        ]

        clean_test, removed_test = _exclude_test_leakage(task_train_all, task_test_all)
        train_records, validation_records = _split_train_and_validation(
            task_train_all,
            validation_ratio=validation_ratio,
            random_seed=random_seed,
        )

        split_rows = {
            "train": _to_rows(train_records, task, "train"),
            "val": _to_rows(validation_records, task, "val"),
            "test": _to_rows(clean_test, task, "test"),
        }
        for split, rows in split_rows.items():
            write_manifest(output_dir / f"{task_name}_{split}.csv", rows)

        summary[task_name] = {
            "train": len(split_rows["train"]),
            "val": len(split_rows["val"]),
            "test": len(split_rows["test"]),
            "removed_test_duplicates": len(removed_test),
            "labels": task.class_to_index,
        }

    with (output_dir / "manifest_summary.json").open("w", encoding="utf-8") as file:
        json.dump(summary, file, ensure_ascii=False, indent=2)
    return summary
