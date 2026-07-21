import QtQuick
import BYD

Item {
    id: root

    width: 0
    height: 0
    visible: false

    function toBoolean(value) {
        if (typeof value === "boolean")
            return value
        const normalized = String(value).toLowerCase()
        return normalized === "true"
                || normalized === "1"
                || normalized === "on"
                || normalized === "open"
                || normalized === "enabled"
    }

    function pageFromName(pageName) {
        switch (String(pageName).toLowerCase()) {
        case "home": return Ui.PAGE_HOME
        case "apps": return Ui.PAGE_APP
        case "ac": return Ui.PAGE_AC
        case "settings": return Ui.PAGE_SETTINGS
        case "vehicle": return Ui.PAGE_VEHICLE
        case "music": return Ui.PAGE_MUSIC
        case "weather": return Ui.PAGE_WEATHER
        case "assistant": return Ui.PAGE_ASSISTANT
        case "contacts": return Ui.PAGE_CONTACTS
        case "video": return Ui.PAGE_VIDEO
        case "calculator": return Ui.PAGE_CALCULATOR
        case "vector": return Ui.PAGE_VECTOR_STUDIO
        case "map": return Ui.PAGE_MAP
        default: return -1
        }
    }

    function pageName(pageIndex) {
        switch (pageIndex) {
        case Ui.PAGE_HOME: return "home"
        case Ui.PAGE_APP: return "apps"
        case Ui.PAGE_AC: return "ac"
        case Ui.PAGE_SETTINGS: return "settings"
        case Ui.PAGE_VEHICLE: return "vehicle"
        case Ui.PAGE_MUSIC: return "music"
        case Ui.PAGE_WEATHER: return "weather"
        case Ui.PAGE_ASSISTANT: return "assistant"
        case Ui.PAGE_CONTACTS: return "contacts"
        case Ui.PAGE_VIDEO: return "video"
        case Ui.PAGE_CALCULATOR: return "calculator"
        case Ui.PAGE_VECTOR_STUDIO: return "vector"
        case Ui.PAGE_MAP: return "map"
        default: return "unknown"
        }
    }

    function finish(callId, toolName, success, message, data) {
        VoiceAssistant.submitToolResult(callId,
                                        toolName,
                                        success,
                                        message,
                                        data || {})
    }

    function executeTool(callId, toolName, toolArguments) {
        let success = true
        let resultText = "操作已完成"
        let resultData = {}

        try {
            switch (toolName) {
            case "get_vehicle_state": {
                resultData = {
                    "page": pageName(Ui.pageIndex),
                    "air_conditioner": {
                        "left_temperature": Ui.acLeftTemperature,
                        "right_temperature": Ui.acRightTemperature,
                        "fan_level": Ui.acFanLevel,
                        "mode": Ui.acModeText
                    },
                    "media": {
                        "playing": MusicPlayer.playing,
                        "title": MusicPlayer.title,
                        "artist": MusicPlayer.artist,
                        "volume": MusicPlayer.volume
                    },
                    "switches": {
                        "fatigue_monitoring": Ui.settingsFatigueReminder,
                        "wlan": Ui.controlCenterWLANStatus,
                        "bluetooth": Ui.controlCenterBluetoothStatus,
                        "positioning": Ui.controlCenterPositionStatus
                    },
                    "dms": {
                        "service_running": DmsSystem.serviceRunning,
                        "face_detected": DmsSystem.faceDetected,
                        "fatigue_level": DmsSystem.fatigueLevel,
                        "status_text": DmsSystem.statusText
                    }
                }
                resultText = "已读取当前中控屏和车辆模拟状态"
                break
            }
            case "open_page": {
                const targetPage = pageFromName(toolArguments.page)
                if (targetPage < 0)
                    throw new Error("未知页面：" + toolArguments.page)
                Ui.navigateTo(targetPage)
                resultData = { "page": pageName(targetPage) }
                resultText = "已打开" + resultData.page + "页面"
                break
            }
            case "set_ac_temperature": {
                const rawTemperature = Number(toolArguments.temperature)
                if (isNaN(rawTemperature))
                    throw new Error("温度参数无效")
                const temperature = Math.max(16, Math.min(32, Math.round(rawTemperature)))
                const zone = String(toolArguments.zone || "both").toLowerCase()
                if (zone === "left" || zone === "both")
                    Ui.acLeftTemperature = temperature
                if (zone === "right" || zone === "both")
                    Ui.acRightTemperature = temperature
                if (zone !== "left" && zone !== "right" && zone !== "both")
                    throw new Error("未知空调区域：" + zone)
                resultData = {
                    "zone": zone,
                    "left_temperature": Ui.acLeftTemperature,
                    "right_temperature": Ui.acRightTemperature
                }
                resultText = "空调温度已设置为 " + temperature + "℃"
                break
            }
            case "set_ac_fan_level": {
                const rawLevel = Number(toolArguments.level)
                if (isNaN(rawLevel))
                    throw new Error("风量参数无效")
                const level = Math.max(0, Math.min(10, Math.round(rawLevel)))
                Ui.acFanLevel = level
                resultData = { "fan_level": Ui.acFanLevel }
                resultText = "空调风量已设置为 " + level + " 档"
                break
            }
            case "set_ac_mode": {
                const mode = String(toolArguments.mode || "normal").toLowerCase()
                if (mode === "dry")
                    Ui.selectAcMode(Ui.AC_MODE_DRY)
                else if (mode === "boost")
                    Ui.selectAcMode(Ui.AC_MODE_BOOST)
                else if (mode === "auto")
                    Ui.selectAcMode(Ui.AC_MODE_AUTO)
                else if (mode === "normal")
                    Ui.selectAcMode(Ui.AC_MODE_NORMAL)
                else
                    throw new Error("未知空调模式：" + mode)
                resultData = { "mode": mode, "mode_text": Ui.acModeText }
                resultText = "空调模式已切换为" + Ui.acModeText
                break
            }
            case "control_music": {
                const action = String(toolArguments.action || "toggle").toLowerCase()
                if (action === "play")
                    MusicPlayer.play()
                else if (action === "pause")
                    MusicPlayer.pause()
                else if (action === "next")
                    MusicPlayer.next()
                else if (action === "previous")
                    MusicPlayer.previous()
                else if (action === "toggle")
                    MusicPlayer.playPause()
                else
                    throw new Error("未知音乐操作：" + action)
                resultData = {
                    "action": action,
                    "playing": MusicPlayer.playing,
                    "title": MusicPlayer.title,
                    "artist": MusicPlayer.artist
                }
                resultText = "音乐操作已执行：" + action
                break
            }
            case "set_media_volume": {
                const rawVolume = Number(toolArguments.volume)
                if (isNaN(rawVolume))
                    throw new Error("音量参数无效")
                const volume = Math.max(0, Math.min(100, Math.round(rawVolume)))
                MusicPlayer.volume = volume
                Ui.controlCenterMediaVolume = volume
                resultData = { "volume": volume }
                resultText = "媒体音量已设置为 " + volume
                break
            }
            case "set_vehicle_switch": {
                const switchName = String(toolArguments.switch_name || "").toLowerCase()
                const enabled = toBoolean(toolArguments.enabled)
                if (switchName === "fatigue_monitoring")
                    Ui.settingsFatigueReminder = enabled
                else if (switchName === "wlan")
                    Ui.controlCenterWLANStatus = enabled
                else if (switchName === "bluetooth")
                    Ui.controlCenterBluetoothStatus = enabled
                else if (switchName === "positioning")
                    Ui.controlCenterPositionStatus = enabled
                else
                    throw new Error("未知车辆开关：" + switchName)
                resultData = { "switch_name": switchName, "enabled": enabled }
                resultText = switchName + " 已" + (enabled ? "开启" : "关闭")
                break
            }
            case "show_toast": {
                resultText = String(toolArguments.message || "操作已完成")
                Ui.showToast(resultText)
                resultData = { "shown": true }
                break
            }
            default:
                throw new Error("不支持的工具：" + toolName)
            }
        } catch (error) {
            success = false
            resultText = String(error)
            resultData = {}
        }

        finish(callId, toolName, success, resultText, resultData)
    }

    Connections {
        target: VoiceAssistant

        function onToolActionRequested(callId, toolName, toolArguments) {
            root.executeTool(callId, toolName, toolArguments)
        }

        function onAgentFailed(message) {
            Ui.showToast(message)
        }
    }
}
