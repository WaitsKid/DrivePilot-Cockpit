from __future__ import annotations

import numpy as np

from dms_backend.inference.roi_extractor import RoiExtractor, centered_box, expand_box
from dms_backend.inference.types import BoundingBox, FaceDetection


def test_centered_box_clips_to_image() -> None:
    box = centered_box(3, 4, 20, 16, 100, 80)
    assert box.x == 0
    assert box.y == 0
    assert box.width > 0
    assert box.height > 0


def test_expand_box_stays_inside_image() -> None:
    box = expand_box(BoundingBox(0, 0, 30, 20), 0.2, image_width=100, image_height=80)
    assert box.x == 0
    assert box.y == 0
    assert box.x2 <= 100
    assert box.y2 <= 80


def test_roi_extractor_returns_non_empty_images() -> None:
    image = np.zeros((240, 320, 3), dtype=np.uint8)
    landmarks = np.array(
        [[125, 100], [185, 100], [155, 130], [135, 160], [175, 160]],
        dtype=np.float32,
    )
    face = FaceDetection(
        box=BoundingBox(90, 60, 130, 150),
        landmarks=landmarks,
        score=0.99,
    )
    rois = RoiExtractor().extract(image, face)
    assert rois.right_eye_image.size > 0
    assert rois.left_eye_image.size > 0
    assert rois.face_image.size > 0
