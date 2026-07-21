from __future__ import annotations

import json
from dataclasses import dataclass, field
from typing import Any, Awaitable, Callable

import httpx

from .config import Settings


class KimiAPIError(RuntimeError):
    pass


@dataclass(slots=True)
class ToolCall:
    id: str
    name: str
    arguments: dict[str, Any]
    raw_arguments: str

    def as_message_item(self) -> dict[str, Any]:
        return {
            "id": self.id,
            "type": "function",
            "function": {
                "name": self.name,
                "arguments": self.raw_arguments,
            },
        }


@dataclass(slots=True)
class ModelTurn:
    content: str = ""
    reasoning_content: str = ""
    tool_calls: list[ToolCall] = field(default_factory=list)
    finish_reason: str = ""

    def as_assistant_message(self) -> dict[str, Any]:
        message: dict[str, Any] = {
            "role": "assistant",
            "content": self.content or "",
        }
        if self.reasoning_content:
            message["reasoning_content"] = self.reasoning_content
        if self.tool_calls:
            message["tool_calls"] = [item.as_message_item() for item in self.tool_calls]
        return message


ReasoningCallback = Callable[[], Awaitable[None]]


class KimiClient:
    def __init__(self, settings: Settings) -> None:
        self.settings = settings
        self._client = httpx.AsyncClient(
            timeout=httpx.Timeout(settings.kimi_timeout_seconds),
            headers={
                "Authorization": f"Bearer {settings.kimi_api_key}",
                "Content-Type": "application/json",
                "User-Agent": "DrivePilot-AI-Agent-Backend/1.0",
            },
        )

    async def close(self) -> None:
        await self._client.aclose()

    async def complete(
        self,
        *,
        messages: list[dict[str, Any]],
        tools: list[dict[str, Any]],
        on_reasoning_started: ReasoningCallback | None = None,
    ) -> ModelTurn:
        payload: dict[str, Any] = {
            "model": self.settings.kimi_model,
            "messages": messages,
            "tools": tools,
            "tool_choice": "auto",
            "stream": True,
            "max_tokens": self.settings.kimi_max_tokens,
        }
        payload["thinking"] = {
            "type": "enabled" if self.settings.kimi_thinking_enabled else "disabled",
            "keep": "all" if self.settings.kimi_keep_reasoning else None,
        }

        content_parts: list[str] = []
        reasoning_parts: list[str] = []
        finish_reason = ""
        tool_buffers: dict[int, dict[str, str]] = {}
        reasoning_notified = False

        try:
            async with self._client.stream(
                "POST",
                self.settings.chat_completions_url,
                json=payload,
            ) as response:
                if response.status_code >= 400:
                    raw = await response.aread()
                    raise KimiAPIError(
                        f"Kimi API HTTP {response.status_code}: {raw.decode('utf-8', errors='replace')}"
                    )

                async for line in response.aiter_lines():
                    if not line.startswith("data:"):
                        continue
                    data = line[5:].strip()
                    if not data or data == "[DONE]":
                        continue
                    try:
                        chunk = json.loads(data)
                    except json.JSONDecodeError as exc:
                        raise KimiAPIError(f"无法解析 Kimi 流式响应：{exc}") from exc

                    choices = chunk.get("choices") or []
                    if not choices:
                        continue
                    choice = choices[0]
                    delta = choice.get("delta") or {}
                    if choice.get("finish_reason"):
                        finish_reason = str(choice["finish_reason"])

                    reasoning = delta.get("reasoning_content")
                    if reasoning:
                        reasoning_parts.append(str(reasoning))
                        if not reasoning_notified and on_reasoning_started is not None:
                            reasoning_notified = True
                            await on_reasoning_started()

                    content = delta.get("content")
                    if content:
                        content_parts.append(str(content))

                    for tool_delta in delta.get("tool_calls") or []:
                        index = int(tool_delta.get("index", 0))
                        buffer = tool_buffers.setdefault(
                            index,
                            {"id": "", "name": "", "arguments": ""},
                        )
                        if tool_delta.get("id"):
                            buffer["id"] = str(tool_delta["id"])
                        function = tool_delta.get("function") or {}
                        if function.get("name"):
                            buffer["name"] += str(function["name"])
                        if function.get("arguments"):
                            buffer["arguments"] += str(function["arguments"])
        except httpx.HTTPError as exc:
            raise KimiAPIError(f"连接 Kimi API 失败：{exc}") from exc

        tool_calls: list[ToolCall] = []
        for index in sorted(tool_buffers):
            item = tool_buffers[index]
            raw_arguments = item["arguments"] or "{}"
            try:
                parsed_arguments = json.loads(raw_arguments)
            except json.JSONDecodeError as exc:
                raise KimiAPIError(
                    f"工具 {item['name']} 的参数不是合法 JSON：{raw_arguments}"
                ) from exc
            if not isinstance(parsed_arguments, dict):
                raise KimiAPIError(f"工具 {item['name']} 的参数必须是 JSON 对象")
            tool_calls.append(
                ToolCall(
                    id=item["id"] or f"tool-{index}",
                    name=item["name"],
                    arguments=parsed_arguments,
                    raw_arguments=raw_arguments,
                )
            )

        return ModelTurn(
            content="".join(content_parts).strip(),
            reasoning_content="".join(reasoning_parts),
            tool_calls=tool_calls,
            finish_reason=finish_reason,
        )
