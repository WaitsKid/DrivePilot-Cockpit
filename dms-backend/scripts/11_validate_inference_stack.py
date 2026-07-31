from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import cv2

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.inference.config import load_inference_config, resolve_project_path
from dms_backend.inference.pipeline import DmsInferencePipeline


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate YuNet + MobileNetV2 ONNX inference stack")
    parser.add_argument("--config", default="configs/inference.yaml")
    args = parser.parse_args()

    config = load_inference_config(args.config)
    model_config = config["models"]
    expected_paths = [
        resolve_project_path(model_config["yunet"]),
        resolve_project_path(model_config["eye_state"]["onnx"]),
        resolve_project_path(model_config["eye_state"]["metadata"]),
        resolve_project_path(model_config["eye_state"]["deployment_config"]),
        resolve_project_path(model_config["yawn_state"]["onnx"]),
        resolve_project_path(model_config["yawn_state"]["metadata"]),
        resolve_project_path(model_config["yawn_state"]["deployment_config"]),
    ]

    print("=== DriveGuard DMS inference validation ===")
    print(f"OpenCV: {cv2.__version__}")
    missing = [path for path in expected_paths if not path.is_file()]
    if missing:
        print("Missing deployment files:")
        for path in missing:
            print(f"  - {path}")
        raise SystemExit(2)

    pipeline = DmsInferencePipeline(args.config)
    smoke_result = pipeline.smoke_test_classifiers()
    print(json.dumps(smoke_result, ensure_ascii=False, indent=2))
    print("YuNet initialization: PASS")
    print("MobileNetV2 ONNX smoke tests: PASS")
    print("Inference stack validation: PASS")


if __name__ == "__main__":
    main()
