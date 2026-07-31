from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def save_training_curves(history: list[dict], output_path: Path) -> None:
    epochs = [item["epoch"] for item in history]

    figure = plt.figure(figsize=(10, 7))
    axis = figure.add_subplot(111)
    axis.plot(epochs, [item["train_loss"] for item in history], label="train loss")
    axis.plot(epochs, [item["val_loss"] for item in history], label="val loss")
    axis.plot(epochs, [item["train_accuracy"] for item in history], label="train accuracy")
    axis.plot(epochs, [item["val_accuracy"] for item in history], label="val accuracy")
    axis.plot(epochs, [item["val_macro_f1"] for item in history], label="val macro F1")
    axis.set_xlabel("Epoch")
    axis.set_ylabel("Metric")
    axis.set_title("Training history")
    axis.grid(True, alpha=0.3)
    axis.legend()
    figure.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output_path, dpi=150)
    plt.close(figure)


def save_confusion_matrix(
    matrix: list[list[int]],
    class_names: dict[int, str],
    output_path: Path,
) -> None:
    values = np.asarray(matrix, dtype=np.int64)
    names = [class_names[index] for index in range(len(class_names))]

    figure = plt.figure(figsize=(6, 5))
    axis = figure.add_subplot(111)
    image = axis.imshow(values, interpolation="nearest")
    figure.colorbar(image, ax=axis)
    axis.set_xticks(range(len(names)), labels=names)
    axis.set_yticks(range(len(names)), labels=names)
    axis.set_xlabel("Predicted")
    axis.set_ylabel("True")
    axis.set_title("Validation confusion matrix")

    threshold = values.max() / 2 if values.size else 0
    for row in range(values.shape[0]):
        for column in range(values.shape[1]):
            axis.text(
                column,
                row,
                str(values[row, column]),
                ha="center",
                va="center",
                color="white" if values[row, column] > threshold else "black",
            )

    figure.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output_path, dpi=150)
    plt.close(figure)
