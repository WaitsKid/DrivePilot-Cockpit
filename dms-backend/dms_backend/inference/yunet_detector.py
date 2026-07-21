from __future__ import annotations

import time
from pathlib import Path

import cv2
import numpy as np

from .types import BoundingBox, FaceDetection


class YuNetDetector:
    """Thin wrapper around OpenCV FaceDetectorYN with largest-face selection."""

    def __init__(
        self,
        model_path: Path,
        *,
        input_size: tuple[int, int] = (320, 320),
        score_threshold: float = 0.85,
        nms_threshold: float = 0.30,
        top_k: int = 5000,
    ) -> None:
        if not model_path.is_file():
            raise FileNotFoundError(f"找不到 YuNet 模型: {model_path}")
        self.model_path = model_path
        self._input_size = tuple(int(value) for value in input_size)
        try:
            self._detector = cv2.FaceDetectorYN.create(
                model=str(model_path),
                config="",
                input_size=self._input_size,
                score_threshold=float(score_threshold),
                nms_threshold=float(nms_threshold),
                top_k=int(top_k),
            )
        except cv2.error as error:
            raise RuntimeError(
                "YuNet 模型加载失败。若 face_detection_yunet_2026may.onnx 与当前 "
                "opencv-python 不兼容，请升级 opencv-python，或改用固定输入的 "
                "face_detection_yunet_2023mar.onnx。"
            ) from error

    def detect_all(self, image_bgr: np.ndarray) -> tuple[list[FaceDetection], float]:
        if image_bgr is None or image_bgr.ndim != 3:
            raise ValueError("YuNet 输入必须是 BGR 彩色图像")
        height, width = image_bgr.shape[:2]
        self._detector.setInputSize((width, height))
        started = time.perf_counter()
        try:
            _, faces = self._detector.detect(image_bgr)
        except cv2.error as error:
            raise RuntimeError(
                "YuNet 推理失败。动态输入模型若与当前 OpenCV 后端不兼容，"
                "请在 stage4.yaml 中改用 face_detection_yunet_2023mar.onnx。"
            ) from error
        elapsed_ms = (time.perf_counter() - started) * 1000.0
        if faces is None:
            return [], elapsed_ms

        detections: list[FaceDetection] = []
        for row in np.asarray(faces, dtype=np.float32):
            x, y, box_width, box_height = row[:4]
            landmarks = row[4:14].reshape(5, 2)
            score = float(row[-1])
            box = _clip_box(
                BoundingBox(
                    x=int(round(x)),
                    y=int(round(y)),
                    width=int(round(box_width)),
                    height=int(round(box_height)),
                ),
                image_width=width,
                image_height=height,
            )
            if box.area <= 0:
                continue
            detections.append(FaceDetection(box=box, landmarks=landmarks, score=score))
        detections.sort(key=lambda item: (item.box.area, item.score), reverse=True)
        return detections, elapsed_ms

    def detect_primary(self, image_bgr: np.ndarray) -> tuple[FaceDetection | None, float]:
        detections, elapsed_ms = self.detect_all(image_bgr)
        return (detections[0] if detections else None), elapsed_ms


def _clip_box(box: BoundingBox, *, image_width: int, image_height: int) -> BoundingBox:
    x1 = max(0, min(box.x, image_width - 1))
    y1 = max(0, min(box.y, image_height - 1))
    x2 = max(x1 + 1, min(box.x2, image_width))
    y2 = max(y1 + 1, min(box.y2, image_height))
    return BoundingBox(x=x1, y=y1, width=x2 - x1, height=y2 - y1)
