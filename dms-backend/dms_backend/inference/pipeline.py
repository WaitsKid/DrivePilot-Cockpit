from __future__ import annotations

import time
from pathlib import Path
from typing import Any

import numpy as np

from .config import load_inference_config, resolve_project_path
from .onnx_classifier import OnnxBinaryClassifier
from .roi_extractor import RoiExtractor
from .types import DmsFrameResult
from .yunet_detector import YuNetDetector


class DmsInferencePipeline:
    """YuNet + two MobileNetV2 ONNX models for per-frame visual cues."""

    def __init__(self, config_path: str | Path | None = None) -> None:
        self.config = load_inference_config(config_path)
        model_config = self.config["models"]
        face_config = self.config.get("face_detector", {})
        runtime_config = self.config.get("runtime", {})
        roi_config = self.config.get("roi", {})

        self.detector = YuNetDetector(
            resolve_project_path(model_config["yunet"]),
            input_size=(
                int(face_config.get("input_width", 320)),
                int(face_config.get("input_height", 320)),
            ),
            score_threshold=float(face_config.get("score_threshold", 0.85)),
            nms_threshold=float(face_config.get("nms_threshold", 0.30)),
            top_k=int(face_config.get("top_k", 5000)),
        )
        threads = int(runtime_config.get("onnx_intra_op_threads", 0))
        self.eye_classifier = _build_classifier(
            "eye_state", model_config["eye_state"], threads
        )
        self.yawn_classifier = _build_classifier(
            "yawn_state", model_config["yawn_state"], threads
        )
        self.roi_extractor = RoiExtractor(
            eye_width_ratio=float(roi_config.get("eye_width_ratio", 0.58)),
            eye_height_ratio=float(roi_config.get("eye_height_ratio", 0.42)),
            minimum_eye_size=int(roi_config.get("minimum_eye_size", 20)),
            face_margin_ratio=float(roi_config.get("face_margin_ratio", 0.10)),
        )

    def analyze(self, frame_bgr: np.ndarray) -> DmsFrameResult:
        started = time.perf_counter()
        face, detection_ms = self.detector.detect_primary(frame_bgr)
        if face is None:
            return DmsFrameResult(
                face_detected=False,
                detection_ms=detection_ms,
                total_ms=(time.perf_counter() - started) * 1000.0,
            )

        warnings: list[str] = []
        try:
            rois = self.roi_extractor.extract(frame_bgr, face)
        except ValueError as error:
            return DmsFrameResult(
                face_detected=True,
                face_score=face.score,
                face_box=face.box,
                detection_ms=detection_ms,
                total_ms=(time.perf_counter() - started) * 1000.0,
                warnings=[str(error)],
            )

        right_eye = self.eye_classifier.predict(rois.right_eye_image)
        left_eye = self.eye_classifier.predict(rois.left_eye_image)
        yawn = self.yawn_classifier.predict(rois.face_image)
        combined_closed_probability = float(
            (right_eye.risk_probability + left_eye.risk_probability) / 2.0
        )
        both_eyes_closed = bool(
            right_eye.risk_detected and left_eye.risk_detected
        )
        if abs(right_eye.risk_probability - left_eye.risk_probability) > 0.55:
            warnings.append("左右眼结果差异较大，可能存在遮挡、侧脸或 ROI 偏移")

        return DmsFrameResult(
            face_detected=True,
            face_score=face.score,
            face_box=face.box,
            right_eye_box=rois.right_eye_box,
            left_eye_box=rois.left_eye_box,
            face_roi_box=rois.face_box,
            right_eye=right_eye,
            left_eye=left_eye,
            yawn=yawn,
            combined_closed_probability=combined_closed_probability,
            both_eyes_closed=both_eyes_closed,
            yawn_detected=yawn.risk_detected,
            detection_ms=detection_ms,
            total_ms=(time.perf_counter() - started) * 1000.0,
            warnings=warnings,
        )

    def smoke_test_classifiers(self) -> dict[str, Any]:
        return {
            "eye_state": self.eye_classifier.smoke_test().probability_map(),
            "yawn_state": self.yawn_classifier.smoke_test().probability_map(),
            "providers": {
                "eye_state": self.eye_classifier.providers,
                "yawn_state": self.yawn_classifier.providers,
            },
        }


def _build_classifier(
    task_name: str,
    raw_config: dict[str, Any],
    threads: int,
) -> OnnxBinaryClassifier:
    return OnnxBinaryClassifier(
        task_name=task_name,
        model_path=resolve_project_path(raw_config["onnx"]),
        metadata_path=resolve_project_path(raw_config["metadata"]),
        deployment_config_path=resolve_project_path(raw_config["deployment_config"]),
        intra_op_threads=threads,
        risk_threshold_override=(
            None
            if raw_config.get("risk_threshold_override") is None
            else float(raw_config["risk_threshold_override"])
        ),
    )
