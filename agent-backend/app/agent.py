from __future__ import annotations

import asyncio
from typing import Any

from .config import Settings
from .local_agent import build_local_plan
from .kimi_client import KimiAPIError, KimiClient, ToolCall
from .protocol import ToolResult
from .session import AgentSession, SendEvent
from .tools import TOOLS, describe_tool_call, tool_title


class AgentRunner:
    def __init__(self, settings: Settings, kimi_client: KimiClient | None) -> None:
        self.settings = settings
        self.kimi_client = kimi_client

    async def run(
        self,
        session: AgentSession,
        user_text: str,
        send_event: SendEvent,
    ) -> None:
        session.messages.append({"role": "user", "content": user_text})
        await send_event(
            {
                "type": "analysis",
                "phase": "understanding",
                "content": "正在处理指令。",
            }
        )

        try:
            if self.kimi_client is None:
                await self._run_local(session, user_text, send_event)
            else:
                await self._run_kimi(session, send_event)
        except asyncio.CancelledError:
            await send_event({"type": "cancelled", "message": "当前 Agent 任务已取消。"})
            raise
        except (KimiAPIError, RuntimeError, ValueError) as exc:
            if self.settings.allow_local_fallback:
                await send_event(
                    {
                        "type": "analysis",
                        "phase": "fallback",
                        "content": f"云端模型调用失败，已切换到本地规则服务：{exc}",
                    }
                )
                await self._run_local(session, user_text, send_event)
            else:
                await send_event({"type": "error", "message": str(exc)})
        finally:
            await send_event({"type": "done"})

    async def _run_kimi(self, session: AgentSession, send_event: SendEvent) -> None:
        assert self.kimi_client is not None

        for step in range(1, self.settings.max_agent_steps + 1):
            reasoning_sent = False

            async def on_reasoning_started() -> None:
                nonlocal reasoning_sent
                if reasoning_sent:
                    return
                reasoning_sent = True
                await send_event(
                    {
                        "type": "analysis",
                        "phase": "thinking",
                        "content": "正在准备操作。",
                    }
                )

            turn = await self.kimi_client.complete(
                messages=session.messages,
                tools=TOOLS,
                on_reasoning_started=on_reasoning_started,
            )
            session.messages.append(turn.as_assistant_message())

            if not turn.tool_calls:
                final_text = turn.content.strip() or "任务处理完成。"
                await send_event({"type": "final", "content": final_text})
                return

            await send_event(
                {
                    "type": "plan",
                    "content": "待执行：" + " → ".join(tool_title(call.name) for call in turn.tool_calls),
                    "tool_count": len(turn.tool_calls),
                }
            )

            for call in turn.tool_calls:
                result = await self._execute_client_tool(session, call, send_event)
                session.messages.append(
                    {
                        "role": "tool",
                        "tool_call_id": call.id,
                        "name": call.name,
                        "content": result.as_model_content(),
                    }
                )

            await send_event(
                {
                    "type": "analysis",
                    "phase": "observation",
                    "content": "已收到车机执行结果。",
                }
            )

        raise RuntimeError(f"Agent 已达到最大执行轮数 {self.settings.max_agent_steps}，为避免循环已停止。")

    async def _execute_client_tool(
        self,
        session: AgentSession,
        call: ToolCall,
        send_event: SendEvent,
    ) -> ToolResult:
        allowed_names = {item["function"]["name"] for item in TOOLS}
        if call.name not in allowed_names:
            return ToolResult(
                call_id=call.id,
                name=call.name,
                success=False,
                message=f"工具 {call.name} 不在白名单中。",
            )

        await send_event(
            {
                "type": "tool_call",
                "call_id": call.id,
                "name": call.name,
                "arguments": call.arguments,
                "display": describe_tool_call(call.name, call.arguments),
            }
        )
        future = session.create_tool_future(call.id)
        try:
            result = await asyncio.wait_for(
                future,
                timeout=self.settings.tool_timeout_seconds,
            )
        except asyncio.TimeoutError:
            session.pending_tools.pop(call.id, None)
            result = ToolResult(
                call_id=call.id,
                name=call.name,
                success=False,
                message=f"Qt 客户端执行 {tool_title(call.name)} 超时。",
            )

        await send_event(
            {
                "type": "observation",
                "call_id": call.id,
                "name": call.name,
                "success": result.success,
                "content": result.message,
                "data": result.data,
            }
        )
        return result

    async def _run_local(
        self,
        session: AgentSession,
        user_text: str,
        send_event: SendEvent,
    ) -> None:
        plan = build_local_plan(user_text)
        await send_event(
            {
                "type": "analysis",
                "phase": "local",
                "content": plan.analysis,
            }
        )
        if not plan.tool_calls:
            await send_event({"type": "final", "content": plan.direct_reply})
            return

        results: list[ToolResult] = []
        await send_event(
            {
                "type": "plan",
                "content": "本地规则：" + " → ".join(tool_title(call.name) for call in plan.tool_calls),
                "tool_count": len(plan.tool_calls),
            }
        )
        for call in plan.tool_calls:
            results.append(await self._execute_client_tool(session, call, send_event))

        succeeded = [item.message for item in results if item.success]
        failed = [item.message for item in results if not item.success]
        parts: list[str] = []
        if succeeded:
            parts.append("已完成：" + "；".join(succeeded))
        if failed:
            parts.append("未完成：" + "；".join(failed))
        await send_event({"type": "final", "content": "\n".join(parts) or "任务处理完成。"})
