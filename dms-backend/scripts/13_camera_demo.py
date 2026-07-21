from __future__ import annotations

import argparse
import json
import sys
import time
from collections import deque
from pathlib import Path

import cv2

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.inference.config import load_stage4_config, resolve_project_path
from dms_backend.inference.pipeline import DmsInferencePipeline
from dms_backend.inference.visualization import draw_result


def main() -> None:
    parser = argparse.ArgumentParser(description="Run local webcam DMS preview")
    parser.add_argument("--config", default="configs/stage4.yaml")
    parser.add_argument("--camera", type=int, default=None)
    args = parser.parse_args()

    config = load_stage4_config(args.config)
    camera_config = config.get("camera", {})
    camera_index = int(camera_config.get("index", 0) if args.camera is None else args.camera)
    width = int(camera_config.get("width", 1280))
    height = int(camera_config.get("height", 720))
    mirror_preview = bool(camera_config.get("mirror_preview", True))
    process_every = max(1, int(camera_config.get("process_every_n_frames", 1)))

    pipeline = DmsInferencePipeline(args.config)
    capture = cv2.VideoCapture(camera_index, cv2.CAP_DSHOW if sys.platform.startswith("win") else 0)
    capture.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    capture.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    if not capture.isOpened():
        raise RuntimeError(f"无法打开摄像头 index={camera_index}")

    output_dir = resolve_project_path(config.get("output_root", "artifacts/stage4")) / "camera"
    output_dir.mkdir(parents=True, exist_ok=True)
    frame_index = 0
    latest_result = None
    timestamps: deque[float] = deque(maxlen=30)

    print("Camera demo started. Press Q/Esc to quit, S to save the current frame.")
    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                print("Camera frame read failed")
                break
            if mirror_preview:
                frame = cv2.flip(frame, 1)

            if frame_index % process_every == 0 or latest_result is None:
                latest_result = pipeline.analyze(frame)
            frame_index += 1

            now = time.perf_counter()
            timestamps.append(now)
            fps = 0.0
            if len(timestamps) >= 2:
                duration = timestamps[-1] - timestamps[0]
                if duration > 0:
                    fps = (len(timestamps) - 1) / duration

            preview = draw_result(frame, latest_result, fps=fps)
            cv2.imshow("DriveGuard DMS Stage 4", preview)
            key = cv2.waitKey(1) & 0xFF
            if key in (27, ord("q"), ord("Q")):
                break
            if key in (ord("s"), ord("S")):
                timestamp = time.strftime("%Y%m%d_%H%M%S")
                image_path = output_dir / f"camera_{timestamp}.jpg"
                json_path = output_dir / f"camera_{timestamp}.json"
                cv2.imwrite(str(image_path), preview)
                with json_path.open("w", encoding="utf-8") as file:
                    json.dump(latest_result.to_dict(), file, ensure_ascii=False, indent=2)
                print(f"Saved: {image_path}")
    finally:
        capture.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
