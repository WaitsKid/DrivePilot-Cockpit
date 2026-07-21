from __future__ import annotations

import asyncio
import logging
from contextlib import asynccontextmanager
from typing import Any

from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from .agent import AgentRunner
from .config import get_settings
from .kimi_client import KimiClient
from .protocol import ClientCommand, ToolResult, ToolResultMessage, UserMessage
from .session import SessionManager

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(name)s | %(message)s",
)
logger = logging.getLogger("drivepilot-agent")

settings = get_settings()
sessions = SessionManager()
kimi_client: KimiClient | None = None
runner: AgentRunner | None = None


@asynccontextmanager
async def lifespan(_: FastAPI):
    global kimi_client, runner
    kimi_client = KimiClient(settings) if settings.model_configured else None
    runner = AgentRunner(settings, kimi_client)
    logger.info(
        "Agent backend starting: model=%s configured=%s",
        settings.kimi_model,
        settings.model_configured,
    )
    try:
        yield
    finally:
        for session_id in list(sessions._sessions):
            sessions.remove(session_id)
        if kimi_client is not None:
            await kimi_client.close()


app = FastAPI(title=settings.app_name, version="1.0.0", lifespan=lifespan)


@app.get("/health")
async def health() -> dict[str, Any]:
    return {
        "status": "ok",
        "service": settings.app_name,
        "model_configured": settings.model_configured,
        "model": settings.kimi_model if settings.model_configured else "local-demo",
        "thinking_enabled": settings.kimi_thinking_enabled,
    }


@app.get("/api/v1/agent/config")
async def public_config() -> dict[str, Any]:
    return {
        "model_configured": settings.model_configured,
        "model": settings.kimi_model if settings.model_configured else "local-demo",
        "thinking_enabled": settings.kimi_thinking_enabled,
        "max_agent_steps": settings.max_agent_steps,
        "tool_timeout_seconds": settings.tool_timeout_seconds,
    }


@app.websocket("/ws/agent/{session_id}")
async def agent_socket(websocket: WebSocket, session_id: str) -> None:
    await websocket.accept()
    session = sessions.get_or_create(session_id)
    send_lock = asyncio.Lock()

    async def send_event(event: dict[str, Any]) -> None:
        async with send_lock:
            await websocket.send_json(event)

    session.send_event = send_event
    await send_event(
        {
            "type": "connected",
            "session_id": session_id,
            "model_configured": settings.model_configured,
            "model": settings.kimi_model if settings.model_configured else "local-demo",
            "thinking_enabled": settings.kimi_thinking_enabled,
        }
    )

    try:
        while True:
            payload = await websocket.receive_json()
            message_type = payload.get("type")

            if message_type == "user_message":
                command = UserMessage.model_validate(payload)
                if session.busy:
                    await send_event({"type": "error", "message": "上一项 Agent 任务仍在执行。"})
                    continue
                assert runner is not None
                task = asyncio.create_task(
                    runner.run(session, command.content.strip(), send_event)
                )
                session.active_task = task

                def report_task_result(completed: asyncio.Task[None]) -> None:
                    if completed.cancelled():
                        return
                    error = completed.exception()
                    if error is not None:
                        logger.error(
                            "Agent task failed: session=%s error=%s",
                            session_id,
                            error,
                        )

                task.add_done_callback(report_task_result)

            elif message_type == "tool_result":
                command = ToolResultMessage.model_validate(payload)
                resolved = session.resolve_tool(
                    ToolResult(
                        call_id=command.call_id,
                        name=command.name,
                        success=command.success,
                        message=command.message,
                        data=command.data,
                    )
                )
                if not resolved:
                    await send_event(
                        {
                            "type": "error",
                            "message": f"未找到等待中的工具调用：{command.call_id}",
                        }
                    )

            elif message_type in {"reset", "cancel", "ping"}:
                command = ClientCommand.model_validate(payload)
                if command.type == "reset":
                    session.cancel_active()
                    session.reset()
                    await send_event({"type": "session_reset"})
                elif command.type == "cancel":
                    session.cancel_active()
                    await send_event({"type": "cancelled", "message": "当前任务已取消。"})
                    await send_event({"type": "done"})
                else:
                    await send_event({"type": "pong"})
            else:
                await send_event({"type": "error", "message": f"未知消息类型：{message_type}"})

    except WebSocketDisconnect:
        logger.info("Qt client disconnected: session=%s", session_id)
    except Exception:
        logger.exception("WebSocket session failed: %s", session_id)
    finally:
        sessions.remove(session_id)
