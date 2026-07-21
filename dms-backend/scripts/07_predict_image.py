from __future__ import annotations

import argparse
import sys
from pathlib import Path

import torch
from PIL import Image

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.training.model_factory import load_checkpoint_model
from dms_backend.training.trainer import resolve_device
from dms_backend.training.transforms import build_transforms


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Predict one image with a Stage 2 checkpoint")
    parser.add_argument("--checkpoint", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--device", default="auto", choices=["auto", "cuda", "cpu"])
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    device = resolve_device(arguments.device)
    model, checkpoint = load_checkpoint_model(arguments.checkpoint, device)
    config = checkpoint["config"]
    transform = build_transforms(config, training=False)

    with Image.open(arguments.image) as image:
        tensor = transform(image.convert("RGB")).unsqueeze(0).to(device)

    with torch.inference_mode():
        probabilities = torch.softmax(model(tensor), dim=1)[0].cpu()

    class_names = {
        int(index): name for index, name in config["task"]["classes"].items()
    }
    prediction = int(probabilities.argmax().item())
    print(f"Task: {config['task_name']}")
    print(f"Prediction: {class_names[prediction]}")
    for index, probability in enumerate(probabilities.tolist()):
        print(f"  {class_names[index]}: {probability:.4f}")


if __name__ == "__main__":
    main()
