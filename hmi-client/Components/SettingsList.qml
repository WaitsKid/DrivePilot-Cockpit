import QtQuick
import QtQuick.Controls
import DrivePilot

Item {
    id: root

    property int contentWidth: root.width * 0.9
    property int contentHeight: 3520
    property int functionValue: Ui.settingsFunctionValue

    readonly property int section1Y: 0
    readonly property int section2Y: 445
    readonly property int section3Y: 965
    readonly property int section4Y: 1500
    readonly property int section5Y: 1885
    readonly property int section6Y: 2270
    readonly property int section7Y: 2660
    readonly property int section8Y: 3050

    signal moveStarted
    signal moveEnded

    function scrollToSection(index) {
        switch (index) {
        case 0: flickable.contentY = section1Y; break
        case 1: flickable.contentY = section2Y; break
        case 2: flickable.contentY = section3Y; break
        case 3: flickable.contentY = section4Y; break
        case 4: flickable.contentY = section5Y; break
        case 5: flickable.contentY = section6Y; break
        case 6: flickable.contentY = section7Y; break
        default: flickable.contentY = section8Y; break
        }
    }

    function syncTabFromContent(contentYValue) {
        var y = contentYValue + 80
        var section = 0
        if (y >= section8Y)
            section = 7
        else if (y >= section7Y)
            section = 6
        else if (y >= section6Y)
            section = 5
        else if (y >= section5Y)
            section = 4
        else if (y >= section4Y)
            section = 3
        else if (y >= section3Y)
            section = 2
        else if (y >= section2Y)
            section = 1
        if (Ui.settingsFunctionValue !== section)
            Ui.settingsFunctionValue = section
    }

    onFunctionValueChanged: scrollToSection(functionValue)

    Flickable {
        id: flickable
        width: root.width
        height: root.height
        clip: true
        contentWidth: root.width
        contentHeight: root.contentHeight
        maximumFlickVelocity: 5000

        onMovementStarted: moveStarted()
        onMovementEnded: moveEnded()
        onContentYChanged: root.syncTabFromContent(contentY)

        rebound: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 420
                easing.type: Easing.OutQuad
            }
        }

        Item {
            id: flickableItem
            width: root.contentWidth
            height: root.contentHeight
            x: 38
            y: 20

            Rectangle {
                id: rect1
                width: parent.width
                height: 400
                radius: 24
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("智能底盘")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 18

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("转向助力模式"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar2 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("舒适")
                            function2Text: qsTr("运动")
                            functionValue: Ui.settingsSteering
                            onFunctionValueChanged: Ui.settingsSteering = functionValue
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("交通环境"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar2 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("城市")
                            function2Text: qsTr("越野")
                            functionValue: Ui.settingsTrafficEnvironment
                            onFunctionValueChanged: Ui.settingsTrafficEnvironment = functionValue
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("制动能量回收"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar3 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("舒适")
                            function2Text: qsTr("标准")
                            function3Text: qsTr("增强")
                            functionValue: Ui.settingsBrakeAssistMode
                            onFunctionValueChanged: Ui.settingsBrakeAssistMode = functionValue
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12

                        Label {
                            width: parent.width - parkingSwitch.width - 12
                            text: qsTr("舒适停车")
                            font.pixelSize: 18
                            color: "#FFFFFF"
                            verticalAlignment: Text.AlignVCenter
                        }

                        StudioSwitch {
                            id: parkingSwitch
                            checked: Ui.settingsParking
                            onToggled: Ui.settingsParking = checked
                        }
                    }
                }
            }

            Rectangle {
                id: rect2
                width: parent.width
                height: 475
                radius: 24
                anchors.top: rect1.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("灯光氛围")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 20

                    // ① 大灯高度调节
                    Column {
                        width: parent.width        // ★ 关键：内层 Column 必须给出确定宽度
                        spacing: 10

                        Item {
                            width: parent.width
                            height: lblLeft.implicitHeight

                            Label {
                                id: lblLeft
                                text: qsTr("大灯高度调节")
                                color: "#FFFFFF"
                                font.pixelSize: 18
                                anchors.left: parent.left
                            }
                            Label {
                                text: Ui.settingsLampHeight + qsTr(" 档")
                                color: "#FFFFFF"
                                font.pixelSize: 18
                                font.bold: true
                                anchors.right: parent.right   // ★ 右对齐用锚，别用弹簧 Item
                            }
                        }

                        ColorSlider {
                            width: parent.width
                            height: 19
                            minValue: 0
                            maxValue: 9
                            value: Ui.settingsLampHeight
                            onValueModified: Ui.settingsLampHeight = value
                        }
                    }

                    // ② 氛围灯区域
                    Column {
                        width: parent.width        // ★ 同样补上
                        spacing: 10
                        Label { text: qsTr("氛围灯区域"); color: "#FFFFFF"; font.pixelSize: 18 }
                        FunctionBar3 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("整车")
                            function2Text: qsTr("前排")
                            function3Text: qsTr("后排")
                        }
                    }

                    // ③④ 开关行保持原样即可，它们的 parent（外层 Column）宽度是确定的
                    Row {
                        width: parent.width
                        spacing: 12
                        Label {
                            width: parent.width - ambientSwitch.width - 12
                            text: qsTr("氛围灯")
                            font.pixelSize: 18
                            color: "#FFFFFF"
                        }
                        StudioSwitch {
                            id: ambientSwitch
                            checked: Ui.settingsAmbientLightEnabled
                            onToggled: Ui.settingsAmbientLightEnabled = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label {
                            width: parent.width - ambientDynamicSwitch.width - 12
                            text: qsTr("动态色彩")
                            font.pixelSize: 18
                            color: "#FFFFFF"
                        }
                        StudioSwitch {
                            id: ambientDynamicSwitch
                            checked: Ui.settingsAmbientDynamic
                            onToggled: Ui.settingsAmbientDynamic = checked
                        }
                    }
                }
            }

            Rectangle {
                id: rect3
                width: parent.width
                height: 490
                radius: 24
                anchors.top: rect2.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("抬头显示")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 18

                    // ① HUD 开关（原本就没问题，不动）
                    Row {
                        width: parent.width
                        spacing: 12
                        Label {
                            width: parent.width - hudSwitch.width - 12
                            text: qsTr("HUD 开关")
                            font.pixelSize: 18
                            color: "#FFFFFF"
                        }
                        StudioSwitch {
                            id: hudSwitch
                            checked: Ui.settingsHudEnabled
                            onToggled: Ui.settingsHudEnabled = checked
                        }
                    }

                    // ② 高度调节
                    Column {
                        width: parent.width        // ★ 补上确定宽度
                        spacing: 8

                        Item {
                            width: parent.width
                            height: heightTitle.implicitHeight

                            Label {
                                id: heightTitle
                                text: qsTr("高度调节")
                                color: "#FFFFFF"
                                font.pixelSize: 18
                                anchors.left: parent.left
                            }
                            Label {
                                text: Ui.settingsHudHeight
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                anchors.right: parent.right   // ★ 锚右边，替代弹簧 Item
                            }
                        }

                        StudioSlider {
                            width: parent.width
                            from: 0
                            to: 10
                            stepSize: 1
                            value: Ui.settingsHudHeight
                            enabled: Ui.settingsHudEnabled
                            onMoved: Ui.settingsHudHeight = Math.round(value)
                        }
                    }

                    // ③ 亮度调节
                    Column {
                        width: parent.width        // ★
                        spacing: 8

                        Item {
                            width: parent.width
                            height: brightnessTitle.implicitHeight

                            Label {
                                id: brightnessTitle
                                text: qsTr("亮度调节")
                                color: "#FFFFFF"
                                font.pixelSize: 18
                                anchors.left: parent.left
                            }
                            Label {
                                text: Ui.settingsHudBrightness
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                anchors.right: parent.right
                            }
                        }

                        StudioSlider {
                            width: parent.width
                            from: 0
                            to: 10
                            stepSize: 1
                            value: Ui.settingsHudBrightness
                            enabled: Ui.settingsHudEnabled
                            onMoved: Ui.settingsHudBrightness = Math.round(value)
                        }
                    }

                    // ④ 旋转调节
                    Column {
                        width: parent.width        // ★
                        spacing: 8

                        Item {
                            width: parent.width
                            height: rotationTitle.implicitHeight

                            Label {
                                id: rotationTitle
                                text: qsTr("旋转调节")
                                color: "#FFFFFF"
                                font.pixelSize: 18
                                anchors.left: parent.left
                            }
                            Label {
                                text: Ui.settingsHudRotation
                                color: "#FFFFFF"
                                font.pixelSize: 16
                                anchors.right: parent.right
                            }
                        }

                        StudioSlider {
                            width: parent.width
                            from: 0
                            to: 10
                            stepSize: 1
                            value: Ui.settingsHudRotation
                            enabled: Ui.settingsHudEnabled
                            onMoved: Ui.settingsHudRotation = Math.round(value)
                        }
                    }
                }
            }

            Rectangle {
                id: rect4
                width: parent.width
                height: 340
                radius: 24
                anchors.top: rect3.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("迎宾")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 20

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("迎宾模式"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar3 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("标准")
                            function2Text: qsTr("灯效")
                            function3Text: qsTr("静默")
                            functionValue: Ui.settingsWelcomeMode
                            onFunctionValueChanged: Ui.settingsWelcomeMode = functionValue
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - welcomeUnlockSwitch.width - 12; text: qsTr("靠近自动解锁"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: welcomeUnlockSwitch
                            checked: Ui.settingsWelcomeUnlock
                            onToggled: Ui.settingsWelcomeUnlock = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - welcomeSoundSwitch.width - 12; text: qsTr("迎宾提示音"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: welcomeSoundSwitch
                            checked: Ui.settingsWelcomeSound
                            onToggled: Ui.settingsWelcomeSound = checked
                        }
                    }
                }
            }

            Rectangle {
                id: rect5
                width: parent.width
                height: 340
                radius: 24
                anchors.top: rect4.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("智能记忆")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 20

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("驾驶员配置"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar3 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("我的")
                            function2Text: qsTr("家庭")
                            function3Text: qsTr("访客")
                            functionValue: Ui.settingsMemoryProfile
                            onFunctionValueChanged: Ui.settingsMemoryProfile = functionValue
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - easyEntrySwitch.width - 12; text: qsTr("座椅便捷进出"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: easyEntrySwitch
                            checked: Ui.settingsEasyEntry
                            onToggled: Ui.settingsEasyEntry = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - mirrorReverseSwitch.width - 12; text: qsTr("倒车后视镜下翻"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: mirrorReverseSwitch
                            checked: Ui.settingsMirrorReverse
                            onToggled: Ui.settingsMirrorReverse = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - mirrorFoldSwitch.width - 12; text: qsTr("锁车自动折叠后视镜"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: mirrorFoldSwitch
                            checked: Ui.settingsMirrorAutoFold
                            onToggled: Ui.settingsMirrorAutoFold = checked
                        }
                    }
                }
            }

            Rectangle {
                id: rect6
                width: parent.width
                height: 345
                radius: 24
                anchors.top: rect5.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("空调")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 18

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - acSyncSwitch.width - 12; text: qsTr("主副驾温区同步"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: acSyncSwitch
                            checked: Ui.settingsAcSync
                            onToggled: Ui.settingsAcSync = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - autoDefogSwitch.width - 12; text: qsTr("自动除雾"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: autoDefogSwitch
                            checked: Ui.settingsAcAutoDefog
                            onToggled: Ui.settingsAcAutoDefog = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - acPurifySwitch.width - 12; text: qsTr("空气净化 / PM2.5"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: acPurifySwitch
                            checked: Ui.settingsAcPurify
                            onToggled: Ui.settingsAcPurify = checked
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 10
                        Label { text: qsTr("香氛强度"); font.pixelSize: 18; color: "#FFFFFF" }
                        FunctionBar3 {
                            width: parent.width
                            height: 42
                            function1Text: qsTr("轻柔")
                            function2Text: qsTr("标准")
                            function3Text: qsTr("浓郁")
                            functionValue: Ui.settingsAcFragrance
                            onFunctionValueChanged: Ui.settingsAcFragrance = functionValue
                        }
                    }
                }
            }

            Rectangle {
                id: rect7
                width: parent.width
                height: 360
                radius: 24
                anchors.top: rect6.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("门窗和锁")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 18

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - doorAutoLockSwitch.width - 12; text: qsTr("行车自动落锁"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: doorAutoLockSwitch
                            checked: Ui.settingsDoorAutoLock
                            onToggled: Ui.settingsDoorAutoLock = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - unlockOnParkSwitch.width - 12; text: qsTr("P 挡自动解锁"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: unlockOnParkSwitch
                            checked: Ui.settingsDoorUnlockOnPark
                            onToggled: Ui.settingsDoorUnlockOnPark = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - childLockSwitch.width - 12; text: qsTr("儿童锁"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: childLockSwitch
                            checked: Ui.settingsChildLock
                            onToggled: Ui.settingsChildLock = checked
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 8
                        Row {
                            width: parent.width
                            Label { text: qsTr("电动尾门开启高度"); color: "#FFFFFF"; font.pixelSize: 18 }
                            Item { width: parent.width - 220; height: 1 }
                            Label { text: Ui.settingsTailgateHeight; color: "#FFFFFF"; font.pixelSize: 16 }
                        }
                        StudioSlider {
                            width: parent.width
                            from: 0
                            to: 10
                            stepSize: 1
                            value: Ui.settingsTailgateHeight
                            onMoved: Ui.settingsTailgateHeight = Math.round(value)
                        }
                    }
                }
            }

            Rectangle {
                id: rect8
                width: parent.width
                height: 360
                radius: 24
                anchors.top: rect7.bottom
                anchors.topMargin: 45
                color: "#26192838"
                border.width: 1
                border.color: "#24FFFFFF"

                Label {
                    text: qsTr("智能提醒")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.left: parent.left
                    anchors.leftMargin: 24
                    anchors.top: parent.top
                    anchors.topMargin: 18
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    anchors.topMargin: 68
                    spacing: 18

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - fatigueSwitch.width - 12; text: qsTr("疲劳驾驶提醒"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: fatigueSwitch
                            checked: Ui.settingsFatigueReminder
                            onToggled: Ui.settingsFatigueReminder = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - speedLimitSwitch.width - 12; text: qsTr("超速提醒"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: speedLimitSwitch
                            checked: Ui.settingsSpeedLimitReminder
                            onToggled: Ui.settingsSpeedLimitReminder = checked
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12
                        Label { width: parent.width - departureSwitch.width - 12; text: qsTr("前车起步提醒"); color: "#FFFFFF"; font.pixelSize: 18 }
                        StudioSwitch {
                            id: departureSwitch
                            checked: Ui.settingsDepartureReminder
                            onToggled: Ui.settingsDepartureReminder = checked
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 8
                        Row {
                            width: parent.width
                            Label { text: qsTr("提醒音量"); color: "#FFFFFF"; font.pixelSize: 18 }
                            Item { width: parent.width - 140; height: 1 }
                            Label { text: Ui.settingsReminderVolume; color: "#FFFFFF"; font.pixelSize: 16 }
                        }
                        StudioSlider {
                            width: parent.width
                            from: 0
                            to: 10
                            stepSize: 1
                            value: Ui.settingsReminderVolume
                            onMoved: Ui.settingsReminderVolume = Math.round(value)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: scrollBar
        anchors.right: flickable.right
        y: getY()
        width: 12
        height: 130
        radius: 23
        color: "#364A5E"

        function getY() {
            var y = flickable.visibleArea.yPosition * flickable.height
            var maxY = flickable.height - height
            if (y < 0)
                return 0
            if (y > maxY)
                return maxY
            return y
        }
    }
}
