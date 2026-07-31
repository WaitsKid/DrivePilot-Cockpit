from __future__ import annotations

import cv2
import numpy as np

from .types import BoundingBox, DmsFrameResult


GREEN = (80, 220, 130)
YELLOW = (40, 210, 255)
RED = (80, 80, 245)
BLUE = (255, 170, 80)
WHITE = (245, 245, 245)
DARK = (22, 28, 38)


def draw_result(
    frame_bgr: np.ndarray,
    result: DmsFrameResult,
    *,
    fps: float | None = None,
) -> np.ndarray:
    output = frame_bgr.copy()
    if not result.face_detected:
        _draw_banner(output, "NO FACE DETECTED", YELLOW)
        _draw_performance(output, result, fps)
        return output

    if result.face_box is not None:
        _draw_box(output, result.face_box, GREEN, 2)
    if result.right_eye_box is not None:
        _draw_box(output, result.right_eye_box, BLUE, 1)
    if result.left_eye_box is not None:
        _draw_box(output, result.left_eye_box, BLUE, 1)

    closed_probability = result.combined_closed_probability
    yawn_probability = result.yawn.risk_probability if result.yawn is not None else 0.0
    risk_active = result.both_eyes_closed or result.yawn_detected
    banner_color = RED if risk_active else GREEN
    banner_text = (
        f"EYES CLOSED {closed_probability:.2f} | YAWN {yawn_probability:.2f}"
    )
    _draw_banner(output, banner_text, banner_color)

    lines = [
        f"face score: {result.face_score:.3f}",
        f"right closed: {_probability(result.right_eye):.3f}",
        f"left closed: {_probability(result.left_eye):.3f}",
        f"yawn: {yawn_probability:.3f}",
    ]
    _draw_text_panel(output, lines)
    _draw_performance(output, result, fps)
    return output


def _probability(prediction) -> float:
    return 0.0 if prediction is None else float(prediction.risk_probability)


def _draw_box(image: np.ndarray, box: BoundingBox, color: tuple[int, int, int], thickness: int) -> None:
    cv2.rectangle(image, (box.x, box.y), (box.x2, box.y2), color, thickness, cv2.LINE_AA)


def _draw_banner(image: np.ndarray, text: str, color: tuple[int, int, int]) -> None:
    width = image.shape[1]
    cv2.rectangle(image, (0, 0), (width, 42), DARK, -1)
    cv2.rectangle(image, (0, 0), (8, 42), color, -1)
    cv2.putText(image, text, (20, 28), cv2.FONT_HERSHEY_DUPLEX, 0.68, WHITE, 1, cv2.LINE_AA)


def _draw_text_panel(image: np.ndarray, lines: list[str]) -> None:
    panel_width = 225
    panel_height = 28 + len(lines) * 25
    x1 = max(0, image.shape[1] - panel_width - 12)
    y1 = 54
    overlay = image.copy()
    cv2.rectangle(overlay, (x1, y1), (image.shape[1] - 12, y1 + panel_height), DARK, -1)
    cv2.addWeighted(overlay, 0.78, image, 0.22, 0, image)
    for index, line in enumerate(lines):
        cv2.putText(
            image,
            line,
            (x1 + 12, y1 + 25 + index * 25),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.52,
            WHITE,
            1,
            cv2.LINE_AA,
        )


def _draw_performance(image: np.ndarray, result: DmsFrameResult, fps: float | None) -> None:
    text = f"DMS {result.total_ms:.1f} ms | face {result.detection_ms:.1f} ms"
    if fps is not None:
        text += f" | {fps:.1f} FPS"
    cv2.putText(
        image,
        text,
        (14, image.shape[0] - 16),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.50,
        WHITE,
        1,
        cv2.LINE_AA,
    )
