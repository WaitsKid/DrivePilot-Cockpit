from __future__ import annotations

from typing import Any, Literal

from pydantic import BaseModel, Field


class UserMessage(BaseModel):
    type: Literal["user_message"]
    content: str = Field(min_length=1, max_length=8000)


class ToolResultMessage(BaseModel):
    type: Literal["tool_result"]
    call_id: str
    name: str
    success: bool
    message: str = ""
    data: dict[str, Any] = Field(default_factory=dict)


class ClientCommand(BaseModel):
    type: Literal["reset", "cancel", "ping"]


class ToolResult:
    def __init__(
        self,
        *,
        call_id: str,
        name: str,
        success: bool,
        message: str,
        data: dict[str, Any] | None = None,
    ) -> None:
        self.call_id = call_id
        self.name = name
        self.success = success
        self.message = message
        self.data = data or {}

    def as_model_content(self) -> str:
        import json

        return json.dumps(
            {
                "success": self.success,
                "message": self.message,
                "data": self.data,
            },
            ensure_ascii=False,
        )
