from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import cv2

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.inference.config import load_stage4_config, resolve_project_path
from dms_backend.inference.pipeline import DmsInferencePipeline
from dms_backend.inference.visualization import draw_result


def main() -> None:
    parser = argparse.ArgumentParser(description="Run offline DMS inference on one image")
    parser.add_argument("--image", required=True, help="Input image path")
    parser.add_argument("--config", default="configs/stage4.yaml")
    parser.add_argument("--show", action="store_true")
    args = parser.parse_args()

    image_path = Path(args.image)
    image = cv2.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"无法读取图片: {image_path}")

    config = load_stage4_config(args.config)
    output_dir = resolve_project_path(config.get("output_root", "artifacts/stage4")) / "images"
    output_dir.mkdir(parents=True, exist_ok=True)
    pipeline = DmsInferencePipeline(args.config)
    result = pipeline.analyze(image)
    annotated = draw_result(image, result)

    output_image = output_dir / f"{image_path.stem}_annotated.jpg"
    output_json = output_dir / f"{image_path.stem}_result.json"
    cv2.imwrite(str(output_image), annotated)
    with output_json.open("w", encoding="utf-8") as file:
        json.dump(result.to_dict(), file, ensure_ascii=False, indent=2)

    print(json.dumps(result.to_dict(), ensure_ascii=False, indent=2))
    print(f"Annotated image: {output_image}")
    print(f"JSON result: {output_json}")

    if args.show:
        cv2.imshow("DriveGuard DMS image inference", annotated)
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
