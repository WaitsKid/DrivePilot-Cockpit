from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

import cv2

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.inference.config import load_inference_config, resolve_project_path
from dms_backend.inference.pipeline import DmsInferencePipeline
from dms_backend.inference.visualization import draw_result


def main() -> None:
    parser = argparse.ArgumentParser(description="Run offline DMS inference on a video")
    parser.add_argument("--video", required=True)
    parser.add_argument("--config", default="configs/inference.yaml")
    parser.add_argument("--process-every", type=int, default=1)
    args = parser.parse_args()

    input_path = Path(args.video)
    capture = cv2.VideoCapture(str(input_path))
    if not capture.isOpened():
        raise FileNotFoundError(f"无法打开视频: {input_path}")

    config = load_inference_config(args.config)
    output_dir = resolve_project_path(config.get("output_root", "artifacts/inference")) / "videos"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_video = output_dir / f"{input_path.stem}_annotated.mp4"
    output_csv = output_dir / f"{input_path.stem}_timeline.csv"

    fps = capture.get(cv2.CAP_PROP_FPS) or 25.0
    width = int(capture.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(capture.get(cv2.CAP_PROP_FRAME_HEIGHT))
    writer = cv2.VideoWriter(
        str(output_video),
        cv2.VideoWriter_fourcc(*"mp4v"),
        fps,
        (width, height),
    )
    if not writer.isOpened():
        raise RuntimeError(f"无法创建输出视频: {output_video}")

    pipeline = DmsInferencePipeline(args.config)
    process_every = max(1, int(args.process_every))
    frame_index = 0
    latest_result = None
    rows: list[dict[str, object]] = []
    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                break
            if frame_index % process_every == 0 or latest_result is None:
                latest_result = pipeline.analyze(frame)
                rows.append(
                    {
                        "frame": frame_index,
                        "time_seconds": frame_index / fps,
                        "face_detected": latest_result.face_detected,
                        "closed_probability": latest_result.combined_closed_probability,
                        "both_eyes_closed": latest_result.both_eyes_closed,
                        "yawn_probability": (
                            latest_result.yawn.risk_probability if latest_result.yawn else 0.0
                        ),
                        "yawn_detected": latest_result.yawn_detected,
                        "total_ms": latest_result.total_ms,
                    }
                )
            writer.write(draw_result(frame, latest_result, fps=fps))
            frame_index += 1
    finally:
        capture.release()
        writer.release()

    with output_csv.open("w", encoding="utf-8-sig", newline="") as file:
        fieldnames = list(rows[0].keys()) if rows else ["frame"]
        writer_csv = csv.DictWriter(file, fieldnames=fieldnames)
        writer_csv.writeheader()
        writer_csv.writerows(rows)

    print(f"Annotated video: {output_video}")
    print(f"Timeline CSV: {output_csv}")
    print(f"Processed frames: {len(rows)} / {frame_index}")


if __name__ == "__main__":
    main()
