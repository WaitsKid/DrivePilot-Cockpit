from __future__ import annotations

from torchvision import transforms
from torchvision.transforms import InterpolationMode

IMAGENET_MEAN = [0.485, 0.456, 0.406]
IMAGENET_STD = [0.229, 0.224, 0.225]


def build_transforms(config: dict, training: bool):
    image_size = int(config["model"]["image_size"])
    task = config["task"]

    if training:
        brightness, contrast, saturation, hue = task["color_jitter"]
        return transforms.Compose(
            [
                transforms.RandomResizedCrop(
                    image_size,
                    scale=tuple(float(value) for value in task["random_resized_crop_scale"]),
                    ratio=(0.90, 1.10),
                    interpolation=InterpolationMode.BILINEAR,
                ),
                transforms.RandomHorizontalFlip(
                    p=float(task["horizontal_flip_probability"])
                ),
                transforms.RandomRotation(
                    degrees=float(task["rotation_degrees"]),
                    interpolation=InterpolationMode.BILINEAR,
                    fill=(0, 0, 0),
                ),
                transforms.ColorJitter(
                    brightness=float(brightness),
                    contrast=float(contrast),
                    saturation=float(saturation),
                    hue=float(hue),
                ),
                transforms.ToTensor(),
                transforms.Normalize(mean=IMAGENET_MEAN, std=IMAGENET_STD),
                transforms.RandomErasing(
                    p=float(task["random_erasing_probability"]),
                    scale=(0.02, 0.08),
                    ratio=(0.4, 2.5),
                    value="random",
                ),
            ]
        )

    resize_size = round(image_size * 232 / 224)
    return transforms.Compose(
        [
            transforms.Resize(
                resize_size,
                interpolation=InterpolationMode.BILINEAR,
            ),
            transforms.CenterCrop(image_size),
            transforms.ToTensor(),
            transforms.Normalize(mean=IMAGENET_MEAN, std=IMAGENET_STD),
        ]
    )
