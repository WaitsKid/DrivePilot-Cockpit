from __future__ import annotations

from functools import lru_cache

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """Runtime configuration loaded from environment variables or ``.env``."""

    app_name: str = "DrivePilot AI Agent Backend"
    host: str = "127.0.0.1"
    port: int = 8770

    kimi_api_key: str = Field(default="", alias="KIMI_API_KEY")
    kimi_base_url: str = Field(
        default="https://api.moonshot.cn/v1",
        alias="KIMI_BASE_URL",
    )
    kimi_model: str = Field(default="kimi-k2.6", alias="KIMI_MODEL")
    kimi_thinking_enabled: bool = Field(default=True, alias="KIMI_THINKING_ENABLED")
    kimi_keep_reasoning: bool = Field(default=True, alias="KIMI_KEEP_REASONING")
    kimi_max_tokens: int = Field(default=32768, alias="KIMI_MAX_TOKENS")
    kimi_timeout_seconds: float = Field(default=120.0, alias="KIMI_TIMEOUT_SECONDS")

    tool_timeout_seconds: float = 20.0
    max_agent_steps: int = 8
    allow_local_fallback: bool = True

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",
        populate_by_name=True,
    )

    @property
    def model_configured(self) -> bool:
        key = self.kimi_api_key.strip()
        return bool(key) and "your" not in key.lower() and "你的" not in key

    @property
    def chat_completions_url(self) -> str:
        base = self.kimi_base_url.rstrip("/")
        if base.endswith("/chat/completions"):
            return base
        return f"{base}/chat/completions"


@lru_cache(maxsize=1)
def get_settings() -> Settings:
    return Settings()
