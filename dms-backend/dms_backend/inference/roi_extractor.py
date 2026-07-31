from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .types import BoundingBox, FaceDetection


@dataclass(frozen=True)
class ExtractedRois:
    right_eye_image: np.ndarray
    left_eye_image: np.ndarray
    face_image: np.ndarray
    right_eye_box: BoundingBox
    left_eye_box: BoundingBox
    face_box: BoundingBox


class RoiExtractor:
    def __init__(
        self,
        *,
        eye_width_ratio: float = 0.58,
        eye_height_ratio: float = 0.42,
        minimum_eye_size: int = 20,
        face_margin_ratio: float = 0.10,
    ) -> None:
        self.eye_width_ratio = float(eye_width_ratio)
        self.eye_height_ratio = float(eye_height_ratio)
        self.minimum_eye_size = int(minimum_eye_size)
        self.face_margin_ratio = float(face_margin_ratio)

    def extract(self, image_bgr: np.ndarray, face: FaceDetection) -> ExtractedRois:
        height, width = image_bgr.shape[:2]
        right_eye = np.asarray(face.right_eye, dtype=np.float32)
        left_eye = np.asarray(face.left_eye, dtype=np.float32)
        inter_eye_distance = float(np.linalg.norm(left_eye - right_eye))
        if inter_eye_distance < 4.0:
            inter_eye_distance = max(4.0, float(face.box.width) * 0.32)

        eye_width = max(self.minimum_eye_size, int(round(inter_eye_distance * self.eye_width_ratio)))
        eye_height = max(self.minimum_eye_size, int(round(inter_eye_distance * self.eye_height_ratio)))
        right_eye_box = centered_box(
            right_eye[0], right_eye[1], eye_width, eye_height, width, height
        )
        left_eye_box = centered_box(
            left_eye[0], left_eye[1], eye_width, eye_height, width, height
        )
        expanded_face_box = expand_box(
            face.box,
            self.face_margin_ratio,
            image_width=width,
            image_height=height,
        )

        right_eye_image = crop_box(image_bgr, right_eye_box)
        left_eye_image = crop_box(image_bgr, left_eye_box)
        face_image = crop_box(image_bgr, expanded_face_box)
        if right_eye_image.size == 0 or left_eye_image.size == 0 or face_image.size == 0:
            raise ValueError("YuNet ROI 裁剪结果为空，请检查人脸是否过于靠近画面边缘")
        return ExtractedRois(
            right_eye_image=right_eye_image,
            left_eye_image=left_eye_image,
            face_image=face_image,
            right_eye_box=right_eye_box,
            left_eye_box=left_eye_box,
            face_box=expanded_face_box,
        )


def centered_box(
    center_x: float,
    center_y: float,
    width: int,
    height: int,
    image_width: int,
    image_height: int,
) -> BoundingBox:
    x1 = int(round(center_x - width / 2))
    y1 = int(round(center_y - height / 2))
    x2 = x1 + int(width)
    y2 = y1 + int(height)
    x1 = max(0, min(x1, image_width - 1))
    y1 = max(0, min(y1, image_height - 1))
    x2 = max(x1 + 1, min(x2, image_width))
    y2 = max(y1 + 1, min(y2, image_height))
    return BoundingBox(x=x1, y=y1, width=x2 - x1, height=y2 - y1)


def expand_box(
    box: BoundingBox,
    margin_ratio: float,
    *,
    image_width: int,
    image_height: int,
) -> BoundingBox:
    margin_x = int(round(box.width * margin_ratio))
    margin_y = int(round(box.height * margin_ratio))
    x1 = max(0, box.x - margin_x)
    y1 = max(0, box.y - margin_y)
    x2 = min(image_width, box.x2 + margin_x)
    y2 = min(image_height, box.y2 + margin_y)
    return BoundingBox(x=x1, y=y1, width=max(1, x2 - x1), height=max(1, y2 - y1))


def crop_box(image: np.ndarray, box: BoundingBox) -> np.ndarray:
    return image[box.y:box.y2, box.x:box.x2].copy()
