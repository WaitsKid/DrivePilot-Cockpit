from __future__ import annotations

from typing import Any


TOOLS: list[dict[str, Any]] = [
    {
        "type": "function",
        "function": {
            "name": "get_vehicle_state",
            "description": "读取当前页面、空调、媒体、网络和疲劳监测状态。涉及当前状态或相对调整时优先调用。",
            "parameters": {"type": "object", "properties": {}, "additionalProperties": False},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "open_page",
            "description": "打开中控屏中的指定页面。",
            "parameters": {
                "type": "object",
                "required": ["page"],
                "properties": {
                    "page": {
                        "type": "string",
                        "enum": [
                            "home", "apps", "ac", "settings", "vehicle", "music",
                            "weather", "assistant", "contacts", "video", "calculator",
                            "vector", "map"
                        ],
                    }
                },
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_ac_temperature",
            "description": "设置空调温度，支持左侧、右侧或两侧。",
            "parameters": {
                "type": "object",
                "required": ["temperature"],
                "properties": {
                    "temperature": {"type": "number", "minimum": 16, "maximum": 32},
                    "zone": {"type": "string", "enum": ["left", "right", "both"], "default": "both"},
                },
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_ac_fan_level",
            "description": "设置空调风量档位。",
            "parameters": {
                "type": "object",
                "required": ["level"],
                "properties": {"level": {"type": "integer", "minimum": 0, "maximum": 10}},
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_ac_mode",
            "description": "切换空调模式。",
            "parameters": {
                "type": "object",
                "required": ["mode"],
                "properties": {"mode": {"type": "string", "enum": ["normal", "dry", "boost", "auto"]}},
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "control_music",
            "description": "控制本地音乐播放。",
            "parameters": {
                "type": "object",
                "required": ["action"],
                "properties": {"action": {"type": "string", "enum": ["play", "pause", "toggle", "next", "previous"]}},
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_media_volume",
            "description": "设置中控媒体音量。",
            "parameters": {
                "type": "object",
                "required": ["volume"],
                "properties": {"volume": {"type": "integer", "minimum": 0, "maximum": 100}},
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_vehicle_switch",
            "description": "打开或关闭疲劳监测、WLAN、蓝牙或定位。",
            "parameters": {
                "type": "object",
                "required": ["switch_name", "enabled"],
                "properties": {
                    "switch_name": {
                        "type": "string",
                        "enum": ["fatigue_monitoring", "wlan", "bluetooth", "positioning"],
                    },
                    "enabled": {"type": "boolean"},
                },
                "additionalProperties": False,
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "show_toast",
            "description": "在中控屏上显示一条短提示。仅在确有必要时调用。",
            "parameters": {
                "type": "object",
                "required": ["message"],
                "properties": {"message": {"type": "string", "maxLength": 120}},
                "additionalProperties": False,
            },
        },
    },
]


TOOL_TITLES = {
    "get_vehicle_state": "读取当前车况",
    "open_page": "打开页面",
    "set_ac_temperature": "设置空调温度",
    "set_ac_fan_level": "设置空调风量",
    "set_ac_mode": "切换空调模式",
    "control_music": "控制音乐播放",
    "set_media_volume": "设置媒体音量",
    "set_vehicle_switch": "修改车辆开关",
    "show_toast": "显示中控提示",
}


def tool_title(name: str) -> str:
    return TOOL_TITLES.get(name, name)


def describe_tool_call(name: str, arguments: dict[str, Any]) -> str:
    title = tool_title(name)
    if name == "set_ac_temperature":
        return f"{title}：{arguments.get('temperature')}℃（{arguments.get('zone', 'both')}）"
    if name == "set_ac_fan_level":
        return f"{title}：{arguments.get('level')} 档"
    if name == "set_ac_mode":
        return f"{title}：{arguments.get('mode')}"
    if name == "open_page":
        return f"{title}：{arguments.get('page')}"
    if name == "control_music":
        return f"{title}：{arguments.get('action')}"
    if name == "set_media_volume":
        return f"{title}：{arguments.get('volume')}"
    if name == "set_vehicle_switch":
        state = "开启" if arguments.get("enabled") else "关闭"
        return f"{title}：{arguments.get('switch_name')} → {state}"
    return title
