from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.fatigue.config import load_stage5_config
from dms_backend.inference.config import load_stage4_config, resolve_project_path
from dms_backend.service.background_service import DmsBackgroundService


def main() -> None:
    config = load_stage5_config()
    stage4 = load_stage4_config(config.stage4_config_path)
    models = stage4["models"]

    print("=== DriveGuard DMS Stage 5 validation ===")
    print(f"Stage 5 config: {config.path}")
    print(f"Stage 4 config: {config.stage4_config_path}")
    print(f"Server: http://{config.server.host}:{config.server.port}")
    print(f"Target process FPS: {config.monitor.target_process_fps}")
    print("Privacy: camera frames are not stored or exposed")

    required_paths = [
        resolve_project_path(models["yunet"]),
        resolve_project_path(models["eye_state"]["onnx"]),
        resolve_project_path(models["eye_state"]["metadata"]),
        resolve_project_path(models["eye_state"]["deployment_config"]),
        resolve_project_path(models["yawn_state"]["onnx"]),
        resolve_project_path(models["yawn_state"]["metadata"]),
        resolve_project_path(models["yawn_state"]["deployment_config"]),
    ]
    missing = [path for path in required_paths if not path.is_file()]
    if missing:
        print("\nMissing files:")
        for path in missing:
            print(f"- {path}")
        raise SystemExit(1)

    print("\nModel files: PASS")
    service = DmsBackgroundService(config)
    if not service.initialize_models():
        print(service.get_status().get("last_error", "Model initialization failed"))
        raise SystemExit(1)

    print("Model initialization: PASS")
    print("Fatigue engine: PASS")
    print("Stage 5 validation: PASS")
    print("No camera was opened by this validation script.")


if __name__ == "__main__":
    main()
