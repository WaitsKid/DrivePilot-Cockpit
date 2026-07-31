from __future__ import annotations

import asyncio
from dataclasses import dataclass, field
from typing import Any, Awaitable, Callable

from .prompts import SYSTEM_PROMPT
from .protocol import ToolResult


SendEvent = Callable[[dict[str, Any]], Awaitable[None]]


@dataclass
class AgentSession:
    session_id: str
    messages: list[dict[str, Any]] = field(
        default_factory=lambda: [{"role": "system", "content": SYSTEM_PROMPT}]
    )
    pending_tools: dict[str, asyncio.Future[ToolResult]] = field(default_factory=dict)
    active_task: asyncio.Task[None] | None = None
    send_event: SendEvent | None = None

    @property
    def busy(self) -> bool:
        return self.active_task is not None and not self.active_task.done()

    def create_tool_future(self, call_id: str) -> asyncio.Future[ToolResult]:
        loop = asyncio.get_running_loop()
        future: asyncio.Future[ToolResult] = loop.create_future()
        self.pending_tools[call_id] = future
        return future

    def resolve_tool(self, result: ToolResult) -> bool:
        future = self.pending_tools.pop(result.call_id, None)
        if future is None or future.done():
            return False
        future.set_result(result)
        return True

    def reset(self) -> None:
        for future in self.pending_tools.values():
            if not future.done():
                future.cancel()
        self.pending_tools.clear()
        self.messages = [{"role": "system", "content": SYSTEM_PROMPT}]

    def cancel_active(self) -> None:
        if self.active_task is not None and not self.active_task.done():
            self.active_task.cancel()
        self.active_task = None
        for future in self.pending_tools.values():
            if not future.done():
                future.cancel()
        self.pending_tools.clear()


class SessionManager:
    def __init__(self) -> None:
        self._sessions: dict[str, AgentSession] = {}

    def get_or_create(self, session_id: str) -> AgentSession:
        session = self._sessions.get(session_id)
        if session is None:
            session = AgentSession(session_id=session_id)
            self._sessions[session_id] = session
        return session

    def remove(self, session_id: str) -> None:
        session = self._sessions.pop(session_id, None)
        if session is not None:
            session.cancel_active()
