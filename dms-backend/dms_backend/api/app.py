from __future__ import annotations

import asyncio
from contextlib import asynccontextmanager
from pathlib import Path
from typing import AsyncIterator

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse

from dms_backend.fatigue.config import Stage5Config, load_stage5_config
from dms_backend.service.background_service import DmsBackgroundService


def create_app(
    config_path: str | Path | None = None,
    service: DmsBackgroundService | None = None,
) -> FastAPI:
    config: Stage5Config = load_stage5_config(config_path)
    runtime_service = service or DmsBackgroundService(config)

    @asynccontextmanager
    async def lifespan(_: FastAPI) -> AsyncIterator[None]:
        await asyncio.to_thread(runtime_service.initialize_models)
        if config.server.auto_start_monitoring and runtime_service.models_ready:
            runtime_service.start()
        try:
            yield
        finally:
            await asyncio.to_thread(runtime_service.shutdown)

    app = FastAPI(
        title="DriveGuard DMS Backend",
        version="5.0.0",
        description=(
            "Local fatigue-monitoring service. Camera frames remain inside the Python "
            "process and are never returned by the API."
        ),
        lifespan=lifespan,
    )
    app.state.dms_service = runtime_service
    app.state.stage5_config = config

    @app.get("/")
    async def root() -> dict[str, object]:
        return {
            "name": "DriveGuard DMS Backend",
            "version": "5.0.0",
            "docs": "/docs",
            "health": "/health",
            "status": "/api/v1/dms/status",
        }

    @app.get("/health")
    async def health() -> JSONResponse:
        status = runtime_service.get_status()
        healthy = bool(status.get("models_ready")) and not bool(status.get("last_error"))
        payload = {
            "ok": healthy,
            "models_ready": bool(status.get("models_ready")),
            "service_running": bool(status.get("service_running")),
            "camera_available": bool(status.get("camera_available")),
            "monitoring_state": status.get("monitoring_state", "unknown"),
            "last_error": status.get("last_error", ""),
        }
        return JSONResponse(payload, status_code=200 if healthy else 503)

    @app.get("/api/v1/dms/status")
    async def get_status() -> dict[str, object]:
        return runtime_service.get_status()

    @app.get("/api/v1/dms/config")
    async def get_config() -> dict[str, object]:
        return runtime_service.get_public_config()

    @app.post("/api/v1/dms/start")
    async def start_monitoring() -> dict[str, object]:
        started = await asyncio.to_thread(runtime_service.start)
        if not started:
            status = runtime_service.get_status()
            raise HTTPException(
                status_code=503,
                detail=status.get("last_error") or "疲劳监测启动失败",
            )
        return runtime_service.get_status()

    @app.post("/api/v1/dms/stop")
    async def stop_monitoring() -> dict[str, object]:
        await asyncio.to_thread(runtime_service.stop)
        return runtime_service.get_status()

    @app.post("/api/v1/dms/reset")
    async def reset_monitoring_state() -> dict[str, object]:
        return runtime_service.reset_state()

    @app.websocket("/api/v1/dms/events")
    async def dms_events(websocket: WebSocket) -> None:
        await websocket.accept()
        revision, payload = runtime_service.store.snapshot()
        await websocket.send_json(payload)
        interval_seconds = max(
            0.05,
            config.server.websocket_push_interval_ms / 1000.0,
        )
        try:
            while True:
                await asyncio.sleep(interval_seconds)
                new_revision, new_payload = runtime_service.store.snapshot()
                if new_revision > revision:
                    revision = new_revision
                    await websocket.send_json(new_payload)
        except WebSocketDisconnect:
            return

    return app
