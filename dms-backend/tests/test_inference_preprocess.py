from __future__ import annotations

import numpy as np

from dms_backend.inference.onnx_classifier import preprocess_bgr, softmax


def test_preprocess_shape_and_type() -> None:
    image = np.zeros((80, 120, 3), dtype=np.uint8)
    tensor = preprocess_bgr(
        image,
        image_size=224,
        mean=np.asarray([0.485, 0.456, 0.406], dtype=np.float32),
        std=np.asarray([0.229, 0.224, 0.225], dtype=np.float32),
    )
    assert tensor.shape == (1, 3, 224, 224)
    assert tensor.dtype == np.float32


def test_softmax_sums_to_one() -> None:
    probabilities = softmax(np.asarray([2.0, 1.0], dtype=np.float32))
    assert np.isclose(float(probabilities.sum()), 1.0)
    assert probabilities[0] > probabilities[1]
