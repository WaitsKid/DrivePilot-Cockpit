from __future__ import annotations

import argparse
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from dms_backend.api.app import create_app
from dms_backend.fatigue.config import load_service_config


def main() -> None:
    parser = argparse.ArgumentParser(description="Run DriveGuard DMS FastAPI service")
    parser.add_argument("--config", default="configs/service.yaml")
    parser.add_argument("--host", default=None)
    parser.add_argument("--port", type=int, default=None)
    args = parser.parse_args()

    try:
        import uvicorn
    except ImportError as error:
        raise RuntimeError(
            "缺少 uvicorn，请先运行: pip install -r requirements-runtime.txt"
        ) from error

    config = load_service_config(args.config)
    app = create_app(args.config)
    uvicorn.run(
        app,
        host=args.host or config.server.host,
        port=args.port or config.server.port,
        log_level=config.server.log_level,
    )


if __name__ == "__main__":
    main()
