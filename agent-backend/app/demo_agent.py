from __future__ import annotations

import re
import uuid
from dataclasses import dataclass
from typing import Any

from .kimi_client import ToolCall


@dataclass(slots=True)
class DemoPlan:
    analysis: str
    tool_calls: list[ToolCall]
    direct_reply: str = ""


def _call(name: str, arguments: dict[str, Any]) -> ToolCall:
    import json

    raw = json.dumps(arguments, ensure_ascii=False)
    return ToolCall(
        id=f"demo-{uuid.uuid4().hex[:10]}",
        name=name,
        arguments=arguments,
        raw_arguments=raw,
    )


def build_demo_plan(text: str) -> DemoPlan:
    normalized = text.strip()
    lower = normalized.lower()
    calls: list[ToolCall] = []

    temperature_match = re.search(r"(?:空调|温度).*?(1[6-9]|2\d|3[0-2])\s*(?:度|℃)?", normalized)
    if temperature_match:
        calls.append(_call("set_ac_temperature", {"temperature": int(temperature_match.group(1)), "zone": "both"}))

    fan_match = re.search(r"(?:风量|风速).*?([0-9]|10)\s*档?", normalized)
    if fan_match:
        calls.append(_call("set_ac_fan_level", {"level": int(fan_match.group(1))}))

    mode_mapping = [
        ("自动", "auto"),
        ("除湿", "dry"),
        ("加强", "boost"),
        ("强力", "boost"),
        ("正常模式", "normal"),
    ]
    for keyword, mode in mode_mapping:
        if keyword in normalized:
            calls.append(_call("set_ac_mode", {"mode": mode}))
            break

    volume_match = re.search(r"(?:音量).*?([0-9]{1,3})", normalized)
    if volume_match:
        calls.append(_call("set_media_volume", {"volume": min(100, int(volume_match.group(1)))}))

    if "下一首" in normalized:
        calls.append(_call("control_music", {"action": "next"}))
    elif "上一首" in normalized:
        calls.append(_call("control_music", {"action": "previous"}))
    elif "暂停" in normalized:
        calls.append(_call("control_music", {"action": "pause"}))
    elif "播放" in normalized and ("音乐" in normalized or "歌" in normalized):
        calls.append(_call("control_music", {"action": "play"}))

    page_keywords = [
        ("空调页面", "ac"), ("音乐页面", "music"), ("天气", "weather"),
        ("地图", "map"), ("导航", "map"), ("车辆设置", "settings"),
        ("设置页面", "settings"), ("联系人", "contacts"), ("视频", "video"),
        ("计算器", "calculator"), ("画图", "vector"), ("应用", "apps"),
        ("首页", "home"),
    ]
    for keyword, page in page_keywords:
        if keyword in normalized and ("打开" in normalized or "进入" in normalized or "切换" in normalized):
            calls.append(_call("open_page", {"page": page}))
            break

    switch_keywords = [
        ("疲劳", "fatigue_monitoring"),
        ("wlan", "wlan"), ("wifi", "wlan"), ("无线网络", "wlan"),
        ("蓝牙", "bluetooth"), ("定位", "positioning"),
    ]
    for keyword, switch_name in switch_keywords:
        if keyword in lower:
            enabled = not any(word in normalized for word in ("关闭", "关掉", "停止", "禁用"))
            if any(word in normalized for word in ("打开", "开启", "启用", "恢复", "关闭", "关掉", "停止", "禁用")):
                calls.append(_call("set_vehicle_switch", {"switch_name": switch_name, "enabled": enabled}))
                break

    if not calls:
        return DemoPlan(
            analysis="本地演示模式没有识别到可执行车机工具，将作为普通对话处理。",
            tool_calls=[],
            direct_reply=(
                "当前后端运行在本地演示模式。请在 .env 中配置 KIMI_API_KEY 后，"
                "即可使用 Kimi 的自然语言理解、深度思考和多步工具调用。"
            ),
        )

    return DemoPlan(
        analysis=f"已从指令中识别出 {len(calls)} 个车机操作，准备按顺序执行。",
        tool_calls=calls,
    )
