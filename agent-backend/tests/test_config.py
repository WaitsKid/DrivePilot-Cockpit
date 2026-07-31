from app.config import Settings


def test_chat_completion_url() -> None:
    settings = Settings(KIMI_BASE_URL="https://api.moonshot.cn/v1")
    assert settings.chat_completions_url.endswith("/v1/chat/completions")


def test_placeholder_key_is_not_configured() -> None:
    settings = Settings(KIMI_API_KEY="你的Kimi API Key")
    assert not settings.model_configured
