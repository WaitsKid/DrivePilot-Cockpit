import torch

from dms_backend.training.model_factory import build_mobilenet_v2


def test_mobilenet_output_shape() -> None:
    config = {
        "model": {
            "pretrained": False,
            "dropout": 0.25,
            "num_classes": 2,
        }
    }
    model = build_mobilenet_v2(config)
    model.eval()
    with torch.inference_mode():
        output = model(torch.zeros(1, 3, 64, 64))
    assert tuple(output.shape) == (1, 2)
