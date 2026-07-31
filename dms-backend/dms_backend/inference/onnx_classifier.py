from __future__ import annotations

import json
import time
from pathlib import Path
from typing import Any

import cv2
import numpy as np

from .types import BinaryPrediction


class OnnxBinaryClassifier:
    def __init__(
        self,
        *,
        task_name: str,
        model_path: Path,
        metadata_path: Path,
        deployment_config_path: Path,
        intra_op_threads: int = 0,
        risk_threshold_override: float | None = None,
    ) -> None:
        try:
            import onnxruntime as ort
        except ImportError as error:
            raise RuntimeError(
                "未安装 onnxruntime，请运行: pip install -r requirements-runtime.txt"
            ) from error

        for path in (model_path, metadata_path, deployment_config_path):
            if not path.is_file():
                raise FileNotFoundError(f"找不到 {task_name} 部署文件: {path}")
        self.task_name = task_name
        self.model_path = model_path
        self.metadata = _read_json(metadata_path)
        self.deployment = _read_json(deployment_config_path)
        self.class_names = [str(value) for value in self.metadata["class_names"]]
        self.risk_class_index = int(self.deployment["risk_class_index"])
        deployment_threshold = float(self.deployment["risk_probability_threshold"])
        self.risk_threshold = (
            deployment_threshold
            if risk_threshold_override is None
            else float(risk_threshold_override)
        )
        preprocess = self.metadata.get("preprocess", {})
        self.image_size = int(preprocess.get("image_size", self.deployment.get("image_size", 224)))
        self.mean = np.asarray(preprocess.get("mean", [0.485, 0.456, 0.406]), dtype=np.float32)
        self.std = np.asarray(preprocess.get("std", [0.229, 0.224, 0.225]), dtype=np.float32)
        self.input_name = str(self.metadata.get("input_name", "images"))
        self.output_name = str(self.metadata.get("output_name", "logits"))

        session_options = ort.SessionOptions()
        if intra_op_threads > 0:
            session_options.intra_op_num_threads = int(intra_op_threads)
        self._session = ort.InferenceSession(
            str(model_path),
            sess_options=session_options,
            providers=["CPUExecutionProvider"],
        )

    @property
    def providers(self) -> list[str]:
        return list(self._session.get_providers())

    def predict(self, image_bgr: np.ndarray) -> BinaryPrediction:
        tensor = preprocess_bgr(
            image_bgr,
            image_size=self.image_size,
            mean=self.mean,
            std=self.std,
        )
        started = time.perf_counter()
        logits = self._session.run([self.output_name], {self.input_name: tensor})[0][0]
        elapsed_ms = (time.perf_counter() - started) * 1000.0
        probabilities = softmax(logits)
        predicted_index = int(np.argmax(probabilities))
        risk_probability = float(probabilities[self.risk_class_index])
        return BinaryPrediction(
            task=self.task_name,
            class_names=list(self.class_names),
            probabilities=[float(value) for value in probabilities],
            predicted_index=predicted_index,
            predicted_label=self.class_names[predicted_index],
            risk_class_index=self.risk_class_index,
            risk_probability=risk_probability,
            risk_threshold=self.risk_threshold,
            risk_detected=risk_probability >= self.risk_threshold,
            inference_ms=elapsed_ms,
        )

    def smoke_test(self) -> BinaryPrediction:
        sample = np.full((self.image_size, self.image_size, 3), 127, dtype=np.uint8)
        return self.predict(sample)


def preprocess_bgr(
    image_bgr: np.ndarray,
    *,
    image_size: int,
    mean: np.ndarray,
    std: np.ndarray,
) -> np.ndarray:
    if image_bgr is None or image_bgr.size == 0:
        raise ValueError("分类器输入图像为空")
    image_rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
    height, width = image_rgb.shape[:2]
    resize_short = int(round(image_size * 232 / 224))
    scale = resize_short / min(height, width)
    resized_width = max(image_size, int(round(width * scale)))
    resized_height = max(image_size, int(round(height * scale)))
    resized = cv2.resize(image_rgb, (resized_width, resized_height), interpolation=cv2.INTER_LINEAR)
    x1 = max(0, (resized_width - image_size) // 2)
    y1 = max(0, (resized_height - image_size) // 2)
    cropped = resized[y1:y1 + image_size, x1:x1 + image_size]
    if cropped.shape[0] != image_size or cropped.shape[1] != image_size:
        cropped = cv2.resize(cropped, (image_size, image_size), interpolation=cv2.INTER_LINEAR)
    tensor = cropped.astype(np.float32) / 255.0
    tensor = (tensor - mean.reshape(1, 1, 3)) / std.reshape(1, 1, 3)
    tensor = np.transpose(tensor, (2, 0, 1))[None, ...]
    return np.ascontiguousarray(tensor, dtype=np.float32)


def softmax(logits: np.ndarray) -> np.ndarray:
    values = np.asarray(logits, dtype=np.float32)
    shifted = values - np.max(values)
    exponentials = np.exp(shifted)
    return exponentials / np.sum(exponentials)


def _read_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as file:
        raw = json.load(file)
    if not isinstance(raw, dict):
        raise ValueError(f"JSON 文件不是对象: {path}")
    return raw
