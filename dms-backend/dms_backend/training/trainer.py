from __future__ import annotations

import json
import time
from contextlib import nullcontext
from pathlib import Path
from typing import Any

import torch
from torch import nn
from torch.optim import AdamW
from torch.optim.lr_scheduler import ReduceLROnPlateau
from torch.utils.data import DataLoader
from tqdm import tqdm

from dms_backend.training.dataset import ManifestImageDataset, compute_class_weights
from dms_backend.training.metrics import compute_classification_metrics
from dms_backend.training.model_factory import (
    build_mobilenet_v2,
    count_parameters,
    set_backbone_trainable,
)
from dms_backend.training.plots import save_confusion_matrix, save_training_curves
from dms_backend.training.reproducibility import seed_everything
from dms_backend.training.transforms import IMAGENET_MEAN, IMAGENET_STD, build_transforms


def resolve_device(device_setting: str) -> torch.device:
    normalized = device_setting.strip().lower()
    if normalized == "auto":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if normalized == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("配置要求 CUDA，但 torch.cuda.is_available() 为 False")
    return torch.device(normalized)


def _create_grad_scaler(enabled: bool):
    try:
        return torch.amp.GradScaler("cuda", enabled=enabled)
    except (AttributeError, TypeError):
        return torch.cuda.amp.GradScaler(enabled=enabled)


def _autocast_context(device: torch.device, enabled: bool):
    if device.type == "cuda" and enabled:
        return torch.autocast(device_type="cuda", dtype=torch.float16)
    return nullcontext()


def _run_epoch(
    *,
    model: nn.Module,
    loader: DataLoader,
    criterion: nn.Module,
    device: torch.device,
    class_names: dict[int, str],
    optimizer: torch.optim.Optimizer | None,
    scaler,
    amp_enabled: bool,
    gradient_clip_norm: float,
    description: str,
) -> dict[str, Any]:
    training = optimizer is not None
    model.train(training)
    if training and not any(parameter.requires_grad for parameter in model.features.parameters()):
        model.features.eval()

    total_loss = 0.0
    sample_count = 0
    targets: list[int] = []
    predictions: list[int] = []

    progress = tqdm(loader, desc=description, unit="batch", leave=False)
    for images, labels, _paths in progress:
        images = images.to(device, non_blocking=True)
        labels = labels.to(device, non_blocking=True)
        batch_size = images.size(0)

        if training:
            optimizer.zero_grad(set_to_none=True)

        with torch.set_grad_enabled(training):
            with _autocast_context(device, amp_enabled):
                logits = model(images)
                loss = criterion(logits, labels)

            if training:
                scaler.scale(loss).backward()
                if gradient_clip_norm > 0:
                    scaler.unscale_(optimizer)
                    torch.nn.utils.clip_grad_norm_(
                        model.parameters(), max_norm=gradient_clip_norm
                    )
                scaler.step(optimizer)
                scaler.update()

        batch_predictions = logits.argmax(dim=1)
        total_loss += float(loss.detach().item()) * batch_size
        sample_count += batch_size
        targets.extend(labels.detach().cpu().tolist())
        predictions.extend(batch_predictions.detach().cpu().tolist())
        progress.set_postfix(loss=f"{loss.item():.4f}")

    metrics = compute_classification_metrics(targets, predictions, class_names)
    return {
        "loss": total_loss / max(1, sample_count),
        "metrics": metrics,
    }


def _save_checkpoint(
    *,
    path: Path,
    model: nn.Module,
    optimizer: torch.optim.Optimizer,
    scheduler: ReduceLROnPlateau,
    epoch: int,
    best_macro_f1: float,
    config: dict,
    metrics: dict,
) -> None:
    payload = {
        "epoch": epoch,
        "model_state_dict": model.state_dict(),
        "optimizer_state_dict": optimizer.state_dict(),
        "scheduler_state_dict": scheduler.state_dict(),
        "best_macro_f1": best_macro_f1,
        "config": config,
        "metrics": metrics,
        "preprocess": {
            "image_size": int(config["model"]["image_size"]),
            "mean": IMAGENET_MEAN,
            "std": IMAGENET_STD,
            "color_space": "RGB",
        },
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(payload, path)


def train_task(config: dict) -> dict[str, Any]:
    task_name = config["task_name"]
    seed = int(config["seed"])
    seed_everything(seed, deterministic=bool(config.get("deterministic", True)))

    device = resolve_device(str(config.get("device", "auto")))
    training_config = config["training"]
    model_config = config["model"]
    class_names = {int(index): name for index, name in config["task"]["classes"].items()}

    manifest_dir = Path(config["manifest_dir"])
    output_dir = Path(config["output_root"]) / task_name
    output_dir.mkdir(parents=True, exist_ok=True)

    train_dataset = ManifestImageDataset(
        manifest_dir / f"{task_name}_train.csv",
        transform=build_transforms(config, training=True),
    )
    val_dataset = ManifestImageDataset(
        manifest_dir / f"{task_name}_val.csv",
        transform=build_transforms(config, training=False),
    )

    batch_size = int(training_config["batch_size"])
    num_workers = int(config.get("num_workers", 0))
    pin_memory = bool(config.get("pin_memory", True)) and device.type == "cuda"
    loader_kwargs = {
        "batch_size": batch_size,
        "num_workers": num_workers,
        "pin_memory": pin_memory,
        "persistent_workers": num_workers > 0,
    }
    generator = torch.Generator().manual_seed(seed)
    train_loader = DataLoader(
        train_dataset,
        shuffle=True,
        generator=generator,
        drop_last=False,
        **loader_kwargs,
    )
    val_loader = DataLoader(
        val_dataset,
        shuffle=False,
        drop_last=False,
        **loader_kwargs,
    )

    model = build_mobilenet_v2(config).to(device)
    freeze_epochs = int(training_config["freeze_backbone_epochs"])
    set_backbone_trainable(model, trainable=freeze_epochs <= 0)

    class_weights = compute_class_weights(
        train_dataset, num_classes=int(model_config["num_classes"])
    ).to(device)
    criterion = nn.CrossEntropyLoss(
        weight=class_weights,
        label_smoothing=float(training_config.get("label_smoothing", 0.0)),
    )
    learning_rate = float(training_config["learning_rate"])
    backbone_learning_rate = learning_rate * float(
        training_config.get("backbone_lr_multiplier", 0.15)
    )
    optimizer = AdamW(
        [
            {"params": model.features.parameters(), "lr": backbone_learning_rate},
            {"params": model.classifier.parameters(), "lr": learning_rate},
        ],
        weight_decay=float(training_config["weight_decay"]),
    )
    scheduler = ReduceLROnPlateau(
        optimizer,
        mode="max",
        factor=float(training_config["scheduler_factor"]),
        patience=int(training_config["scheduler_patience"]),
        min_lr=float(training_config["min_learning_rate"]),
    )

    amp_enabled = bool(config.get("amp", True)) and device.type == "cuda"
    scaler = _create_grad_scaler(amp_enabled)
    total_parameters, trainable_parameters = count_parameters(model)

    print(f"\n=== Training {task_name} ===")
    print(f"Device: {device}")
    print(f"Train samples: {len(train_dataset)}")
    print(f"Validation samples: {len(val_dataset)}")
    print(f"Class counts: {dict(train_dataset.label_counts())}")
    print(f"Class weights: {[round(value, 4) for value in class_weights.cpu().tolist()]}")
    print(f"Parameters: total={total_parameters:,}, trainable={trainable_parameters:,}")
    print(f"AMP: {amp_enabled}")

    history: list[dict[str, Any]] = []
    best_macro_f1 = -1.0
    epochs_without_improvement = 0
    backbone_unfrozen = freeze_epochs <= 0
    start_time = time.perf_counter()

    for epoch_index in range(int(training_config["epochs"])):
        epoch = epoch_index + 1
        if not backbone_unfrozen and epoch > freeze_epochs:
            set_backbone_trainable(model, trainable=True)
            backbone_unfrozen = True
            total_parameters, trainable_parameters = count_parameters(model)
            print(f"Epoch {epoch}: backbone unfrozen, trainable={trainable_parameters:,}")

        train_result = _run_epoch(
            model=model,
            loader=train_loader,
            criterion=criterion,
            device=device,
            class_names=class_names,
            optimizer=optimizer,
            scaler=scaler,
            amp_enabled=amp_enabled,
            gradient_clip_norm=float(training_config["gradient_clip_norm"]),
            description=f"{task_name} train {epoch}",
        )
        val_result = _run_epoch(
            model=model,
            loader=val_loader,
            criterion=criterion,
            device=device,
            class_names=class_names,
            optimizer=None,
            scaler=scaler,
            amp_enabled=amp_enabled,
            gradient_clip_norm=0.0,
            description=f"{task_name} val {epoch}",
        )

        val_metrics = val_result["metrics"]
        scheduler.step(val_metrics.macro_f1)
        backbone_lr = float(optimizer.param_groups[0]["lr"])
        classifier_lr = float(optimizer.param_groups[1]["lr"])
        epoch_record = {
            "epoch": epoch,
            "backbone_learning_rate": backbone_lr,
            "classifier_learning_rate": classifier_lr,
            "train_loss": train_result["loss"],
            "train_accuracy": train_result["metrics"].accuracy,
            "train_macro_f1": train_result["metrics"].macro_f1,
            "val_loss": val_result["loss"],
            "val_accuracy": val_metrics.accuracy,
            "val_macro_f1": val_metrics.macro_f1,
        }
        history.append(epoch_record)

        print(
            f"Epoch {epoch:02d} | "
            f"train loss={train_result['loss']:.4f} acc={train_result['metrics'].accuracy:.4f} | "
            f"val loss={val_result['loss']:.4f} acc={val_metrics.accuracy:.4f} "
            f"F1={val_metrics.macro_f1:.4f} | "
            f"lr(backbone/head)={backbone_lr:.2e}/{classifier_lr:.2e}"
        )

        metrics_payload = val_metrics.to_dict()
        _save_checkpoint(
            path=output_dir / "last.pt",
            model=model,
            optimizer=optimizer,
            scheduler=scheduler,
            epoch=epoch,
            best_macro_f1=max(best_macro_f1, val_metrics.macro_f1),
            config=config,
            metrics=metrics_payload,
        )

        if val_metrics.macro_f1 > best_macro_f1 + 1e-6:
            best_macro_f1 = val_metrics.macro_f1
            epochs_without_improvement = 0
            _save_checkpoint(
                path=output_dir / "best.pt",
                model=model,
                optimizer=optimizer,
                scheduler=scheduler,
                epoch=epoch,
                best_macro_f1=best_macro_f1,
                config=config,
                metrics=metrics_payload,
            )
            with (output_dir / "best_metrics.json").open("w", encoding="utf-8") as file:
                json.dump(metrics_payload, file, ensure_ascii=False, indent=2)
            save_confusion_matrix(
                val_metrics.confusion_matrix,
                class_names,
                output_dir / "validation_confusion_matrix.png",
            )
        else:
            epochs_without_improvement += 1

        with (output_dir / "history.json").open("w", encoding="utf-8") as file:
            json.dump(history, file, ensure_ascii=False, indent=2)
        save_training_curves(history, output_dir / "training_curves.png")

        if epochs_without_improvement >= int(training_config["early_stopping_patience"]):
            print(
                f"Early stopping: validation macro F1 连续 "
                f"{epochs_without_improvement} 个 epoch 未提升"
            )
            break

    elapsed_seconds = time.perf_counter() - start_time
    summary = {
        "task": task_name,
        "device": str(device),
        "best_validation_macro_f1": best_macro_f1,
        "epochs_completed": len(history),
        "elapsed_seconds": elapsed_seconds,
        "best_checkpoint": str((output_dir / "best.pt").resolve()),
    }
    with (output_dir / "training_summary.json").open("w", encoding="utf-8") as file:
        json.dump(summary, file, ensure_ascii=False, indent=2)

    print(f"Best validation macro F1: {best_macro_f1:.4f}")
    print(f"Best checkpoint: {output_dir / 'best.pt'}")
    return summary
