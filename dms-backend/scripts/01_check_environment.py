from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

import platform


def main() -> None:
    print("=== DriveGuard DMS environment ===")
    print("Python:", sys.version.replace("\n", " "))
    print("Platform:", platform.platform())

    try:
        import torch
        import torchvision
    except ImportError as error:
        raise SystemExit(
            "没有检测到 torch/torchvision。请先按 README 的 PyTorch 安装步骤执行。"
        ) from error

    print("PyTorch:", torch.__version__)
    print("Torchvision:", torchvision.__version__)
    print("CUDA available:", torch.cuda.is_available())
    if torch.cuda.is_available():
        print("CUDA device:", torch.cuda.get_device_name(0))
    else:
        print("Training device: CPU（能训练，但速度会更慢）")

    tensor = torch.rand(2, 3)
    print("Tensor smoke test:", tensor.shape, tensor.dtype)
    print("Environment check: PASS")


if __name__ == "__main__":
    main()
