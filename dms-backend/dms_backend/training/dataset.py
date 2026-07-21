from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path
from typing import Callable

import torch
from PIL import Image, ImageFile
from torch.utils.data import Dataset

ImageFile.LOAD_TRUNCATED_IMAGES = False


class ManifestImageDataset(Dataset):
    """Read image paths and binary labels from a Stage 1 manifest CSV."""

    def __init__(
        self,
        manifest_path: Path,
        transform: Callable | None = None,
    ) -> None:
        self.manifest_path = manifest_path.resolve()
        self.transform = transform
        self.rows = self._read_manifest(self.manifest_path)
        if not self.rows:
            raise ValueError(f"清单中没有样本: {self.manifest_path}")

    @staticmethod
    def _read_manifest(path: Path) -> list[dict[str, str]]:
        if not path.is_file():
            raise FileNotFoundError(
                f"找不到训练清单: {path}\n请先运行 Stage 1 的 03_build_manifests.py"
            )

        with path.open("r", encoding="utf-8-sig", newline="") as file:
            rows = list(csv.DictReader(file))

        required = {"path", "label", "class_name", "task", "split", "sha256"}
        if rows and not required.issubset(rows[0]):
            missing = sorted(required - set(rows[0]))
            raise ValueError(f"清单缺少字段: {missing}")
        return rows

    def __len__(self) -> int:
        return len(self.rows)

    def __getitem__(self, index: int) -> tuple[torch.Tensor, int, str]:
        row = self.rows[index]
        image_path = Path(row["path"])
        try:
            with Image.open(image_path) as image:
                image = image.convert("RGB")
                if self.transform is not None:
                    image = self.transform(image)
        except (OSError, ValueError) as error:
            raise RuntimeError(f"训练时无法读取图片: {image_path}: {error}") from error

        return image, int(row["label"]), str(image_path)

    def label_counts(self) -> Counter[int]:
        return Counter(int(row["label"]) for row in self.rows)


def compute_class_weights(dataset: ManifestImageDataset, num_classes: int) -> torch.Tensor:
    """Inverse-frequency weights normalized to an average value of one."""
    counts = dataset.label_counts()
    total = sum(counts.values())
    if total == 0:
        raise ValueError("训练集为空，无法计算类别权重")

    weights = []
    for class_index in range(num_classes):
        count = counts.get(class_index, 0)
        if count == 0:
            raise ValueError(f"训练集中缺少类别 {class_index}")
        weights.append(total / (num_classes * count))
    return torch.tensor(weights, dtype=torch.float32)
