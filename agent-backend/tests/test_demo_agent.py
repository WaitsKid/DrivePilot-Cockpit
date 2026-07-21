from app.demo_agent import build_demo_plan


def test_temperature_and_page_plan() -> None:
    plan = build_demo_plan("把空调调到22度并打开空调页面")
    assert [item.name for item in plan.tool_calls] == ["set_ac_temperature", "open_page"]
    assert plan.tool_calls[0].arguments["temperature"] == 22


def test_music_and_volume_plan() -> None:
    plan = build_demo_plan("播放下一首音乐，音量调到45")
    names = [item.name for item in plan.tool_calls]
    assert "set_media_volume" in names
    assert "control_music" in names


def test_no_tool_uses_direct_reply() -> None:
    plan = build_demo_plan("你好")
    assert plan.tool_calls == []
    assert plan.direct_reply
