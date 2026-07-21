from __future__ import annotations

import csv
import json
import shutil
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np
import torch
from PIL import Image, ImageDraw, ImageFont
from torch.utils.data import DataLoader

from dms_backend.training.dataset import ManifestImageDataset
from dms_backend.training.metrics import compute_classification_metrics
from dms_backend.training.model_factory import load_checkpoint_model
from dms_backend.training.plots import save_confusion_matrix
from dms_backend.training.trainer import resolve_device
from dms_backend.training.transforms import build_transforms


@dataclass(frozen=True)
class PredictionRecord:
    path: str
    target: int
    prediction: int
    confidence: float
    probabilities: list[float]


def _load_stage3_config(path: Path) -> dict[str, Any]:
    import yaml

    with path.open("r", encoding="utf-8") as file:
        raw = yaml.safe_load(file)
    if not isinstance(raw, dict):
        raise ValueError(f"Stage 3 配置不是有效对象: {path}")
    return raw


def _make_loader(
    manifest_path: Path,
    transform,
    batch_size: int,
    num_workers: int,
    pin_memory: bool,
) -> tuple[ManifestImageDataset, DataLoader]:
    dataset = ManifestImageDataset(manifest_path, transform=transform)
    loader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=False,
        num_workers=num_workers,
        pin_memory=pin_memory,
        persistent_workers=num_workers > 0,
        drop_last=False,
    )
    return dataset, loader


def collect_predictions(
    *,
    model: torch.nn.Module,
    loader: DataLoader,
    device: torch.device,
) -> tuple[list[PredictionRecord], float]:
    records: list[PredictionRecord] = []
    started = time.perf_counter()

    with torch.inference_mode():
        for images, labels, paths in loader:
            images = images.to(device, non_blocking=True)
            logits = model(images)
            probabilities = torch.softmax(logits, dim=1).cpu()
            predictions = probabilities.argmax(dim=1)
            confidences = probabilities.max(dim=1).values

            for index in range(len(paths)):
                records.append(
                    PredictionRecord(
                        path=str(paths[index]),
                        target=int(labels[index]),
                        prediction=int(predictions[index]),
                        confidence=float(confidences[index]),
                        probabilities=[float(value) for value in probabilities[index].tolist()],
                    )
                )

    elapsed = time.perf_counter() - started
    return records, elapsed


def _metrics_from_records(records: list[PredictionRecord], class_names: dict[int, str]) -> dict[str, Any]:
    metrics = compute_classification_metrics(
        [record.target for record in records],
        [record.prediction for record in records],
        class_names,
    )
    return metrics.to_dict()


def _predictions_from_risk_threshold(
    records: list[PredictionRecord],
    *,
    risk_class_index: int,
    threshold: float,
) -> list[int]:
    safe_class_index = 1 - risk_class_index
    return [
        risk_class_index
        if record.probabilities[risk_class_index] >= threshold
        else safe_class_index
        for record in records
    ]


def find_best_risk_threshold(
    records: list[PredictionRecord],
    *,
    class_names: dict[int, str],
    risk_class_index: int,
    minimum: float,
    maximum: float,
    step: float,
) -> tuple[float, dict[str, Any]]:
    if not records:
        raise ValueError("验证集为空，无法选择阈值")

    targets = [record.target for record in records]
    best_threshold = 0.5
    best_metrics: dict[str, Any] | None = None
    best_score = -1.0
    best_risk_recall = -1.0

    threshold = minimum
    while threshold <= maximum + 1e-9:
        predictions = _predictions_from_risk_threshold(
            records,
            risk_class_index=risk_class_index,
            threshold=threshold,
        )
        metrics = compute_classification_metrics(targets, predictions, class_names).to_dict()
        risk_name = class_names[risk_class_index]
        score = float(metrics["macro_f1"])
        risk_recall = float(metrics["per_class"][risk_name]["recall"])

        if score > best_score + 1e-12 or (
            abs(score - best_score) <= 1e-12 and risk_recall > best_risk_recall
        ):
            best_threshold = round(float(threshold), 6)
            best_metrics = metrics
            best_score = score
            best_risk_recall = risk_recall
        threshold += step

    assert best_metrics is not None
    return best_threshold, best_metrics


def _write_predictions_csv(
    output_path: Path,
    records: list[PredictionRecord],
    class_names: dict[int, str],
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(
            [
                "path",
                "target_index",
                "target_name",
                "prediction_index",
                "prediction_name",
                "confidence",
                *[f"probability_{class_names[index]}" for index in sorted(class_names)],
            ]
        )
        for record in records:
            writer.writerow(
                [
                    record.path,
                    record.target,
                    class_names[record.target],
                    record.prediction,
                    class_names[record.prediction],
                    f"{record.confidence:.8f}",
                    *[f"{value:.8f}" for value in record.probabilities],
                ]
            )


def _safe_font(size: int):
    try:
        return ImageFont.truetype("arial.ttf", size=size)
    except OSError:
        return ImageFont.load_default()


def _save_misclassified_grid(
    *,
    records: list[PredictionRecord],
    class_names: dict[int, str],
    output_path: Path,
    maximum_images: int = 24,
) -> None:
    mistakes = [record for record in records if record.target != record.prediction]
    mistakes.sort(key=lambda item: item.confidence, reverse=True)
    mistakes = mistakes[:maximum_images]

    if not mistakes:
        canvas = Image.new("RGB", (760, 120), "white")
        draw = ImageDraw.Draw(canvas)
        draw.text((24, 44), "No misclassified samples on this split.", fill="black", font=_safe_font(20))
        output_path.parent.mkdir(parents=True, exist_ok=True)
        canvas.save(output_path)
        return

    tile_width = 220
    tile_height = 190
    columns = 4
    rows = (len(mistakes) + columns - 1) // columns
    canvas = Image.new("RGB", (columns * tile_width, rows * tile_height), "#ECEFF3")
    draw = ImageDraw.Draw(canvas)
    font = _safe_font(14)

    for index, record in enumerate(mistakes):
        row = index // columns
        column = index % columns
        x = column * tile_width
        y = row * tile_height
        try:
            with Image.open(record.path) as image:
                image = image.convert("RGB")
                image.thumbnail((200, 130))
                image_x = x + (tile_width - image.width) // 2
                image_y = y + 8
                canvas.paste(image, (image_x, image_y))
        except OSError:
            draw.rectangle((x + 10, y + 10, x + 210, y + 135), outline="red", width=2)
            draw.text((x + 18, y + 62), "Unreadable", fill="red", font=font)

        label = (
            f"true={class_names[record.target]}\n"
            f"pred={class_names[record.prediction]}\n"
            f"conf={record.confidence:.3f}"
        )
        draw.multiline_text((x + 10, y + 140), label, fill="black", font=font, spacing=2)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path)


def _copy_top_errors(
    records: list[PredictionRecord],
    output_dir: Path,
    maximum_files: int = 50,
) -> int:
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    mistakes = [record for record in records if record.target != record.prediction]
    mistakes.sort(key=lambda item: item.confidence, reverse=True)
    copied = 0
    for index, record in enumerate(mistakes[:maximum_files]):
        source = Path(record.path)
        if not source.is_file():
            continue
        destination = output_dir / f"{index:03d}_true{record.target}_pred{record.prediction}_{source.name}"
        shutil.copy2(source, destination)
        copied += 1
    return copied


def evaluate_task(
    *,
    stage3_config_path: Path,
    task_name: str,
    checkpoint_path: Path | None = None,
) -> dict[str, Any]:
    config = _load_stage3_config(stage3_config_path)
    task_config = config["tasks"].get(task_name)
    if not isinstance(task_config, dict):
        raise ValueError(f"Stage 3 配置中不存在任务: {task_name}")

    checkpoint_root = Path(config["checkpoint_root"])
    checkpoint_path = checkpoint_path or checkpoint_root / task_name / "best.pt"
    device = resolve_device(str(config.get("device", "auto")))
    model, checkpoint = load_checkpoint_model(checkpoint_path, device)
    training_config = checkpoint["config"]
    class_names = {
        int(index): name for index, name in training_config["task"]["classes"].items()
    }
    transform = build_transforms(training_config, training=False)

    batch_size = int(config.get("batch_size", 64))
    num_workers = int(config.get("num_workers", 0))
    pin_memory = bool(config.get("pin_memory", True)) and device.type == "cuda"
    manifest_dir = Path(config["manifest_dir"])

    _val_dataset, val_loader = _make_loader(
        manifest_dir / f"{task_name}_val.csv",
        transform,
        batch_size,
        num_workers,
        pin_memory,
    )
    test_dataset, test_loader = _make_loader(
        manifest_dir / f"{task_name}_test.csv",
        transform,
        batch_size,
        num_workers,
        pin_memory,
    )

    val_records, val_elapsed = collect_predictions(model=model, loader=val_loader, device=device)
    test_records, test_elapsed = collect_predictions(model=model, loader=test_loader, device=device)

    threshold, threshold_val_metrics = find_best_risk_threshold(
        val_records,
        class_names=class_names,
        risk_class_index=int(task_config["risk_class_index"]),
        minimum=float(task_config.get("threshold_min", 0.2)),
        maximum=float(task_config.get("threshold_max", 0.8)),
        step=float(task_config.get("threshold_step", 0.01)),
    )

    argmax_metrics = _metrics_from_records(test_records, class_names)
    threshold_predictions = _predictions_from_risk_threshold(
        test_records,
        risk_class_index=int(task_config["risk_class_index"]),
        threshold=threshold,
    )
    threshold_metrics = compute_classification_metrics(
        [record.target for record in test_records],
        threshold_predictions,
        class_names,
    ).to_dict()

    output_dir = Path(config["evaluation_root"]) / task_name
    output_dir.mkdir(parents=True, exist_ok=True)
    _write_predictions_csv(output_dir / "test_predictions.csv", test_records, class_names)
    save_confusion_matrix(
        argmax_metrics["confusion_matrix"],
        class_names,
        output_dir / "test_confusion_matrix_argmax.png",
    )
    save_confusion_matrix(
        threshold_metrics["confusion_matrix"],
        class_names,
        output_dir / "test_confusion_matrix_threshold.png",
    )
    _save_misclassified_grid(
        records=test_records,
        class_names=class_names,
        output_path=output_dir / "misclassified_grid.png",
    )
    copied_errors = _copy_top_errors(test_records, output_dir / "misclassified_samples")

    summary = {
        "task": task_name,
        "checkpoint": str(checkpoint_path.resolve()),
        "device": str(device),
        "test_samples": len(test_dataset),
        "validation_inference_seconds": val_elapsed,
        "test_inference_seconds": test_elapsed,
        "test_ms_per_image": test_elapsed * 1000.0 / max(1, len(test_dataset)),
        "argmax_metrics": argmax_metrics,
        "risk_class_index": int(task_config["risk_class_index"]),
        "risk_class_name": class_names[int(task_config["risk_class_index"])],
        "selected_risk_threshold": threshold,
        "threshold_validation_metrics": threshold_val_metrics,
        "threshold_test_metrics": threshold_metrics,
        "misclassified_count": sum(record.target != record.prediction for record in test_records),
        "copied_error_samples": copied_errors,
    }
    with (output_dir / "test_report.json").open("w", encoding="utf-8") as file:
        json.dump(summary, file, ensure_ascii=False, indent=2)

    deployment_config = {
        "task": task_name,
        "class_names": class_names,
        "risk_class_index": int(task_config["risk_class_index"]),
        "risk_class_name": class_names[int(task_config["risk_class_index"])],
        "risk_probability_threshold": threshold,
        "image_size": int(training_config["model"]["image_size"]),
        "preprocess": checkpoint.get("preprocess", {}),
    }
    with (output_dir / "deployment_config.json").open("w", encoding="utf-8") as file:
        json.dump(deployment_config, file, ensure_ascii=False, indent=2)

    print(f"\n=== Test evaluation: {task_name} ===")
    print(f"Samples: {len(test_dataset)}")
    print(f"Argmax accuracy: {argmax_metrics['accuracy']:.4f}")
    print(f"Argmax macro F1: {argmax_metrics['macro_f1']:.4f}")
    print(f"Risk class: {deployment_config['risk_class_name']}")
    print(f"Selected risk threshold from validation: {threshold:.2f}")
    print(f"Threshold test macro F1: {threshold_metrics['macro_f1']:.4f}")
    print(f"Misclassified: {summary['misclassified_count']}")
    print(f"Report: {output_dir / 'test_report.json'}")
    return summary
