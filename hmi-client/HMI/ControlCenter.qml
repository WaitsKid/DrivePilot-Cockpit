import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0
    // 背景
    Image {
        id: backgroundImage
        anchors.fill: parent
        source: "qrc:/Images/ControlCenter/background.png"
        fillMode: Image.PreserveAspectFit

        // 向上滑动区域
        SwipeArea {
            id: upSwipeArea
            anchors.fill: parent

            onSwipeUp: {
                hide()
            }
        }
    }

    // 背景高斯模糊
    GaussianBlur {
        anchors.fill: backgroundImage
        source: backgroundImage
        radius: 8
        samples: 16
    }
    function show() {
        if (root.y < 0) {
            showMoveAnimation.restart()
            showOpacityAnimation.restart()
        }
    }

    NumberAnimation {
        id: showOpacityAnimation
        target: root
        property: "opacity"
        from: 0
        to: 1
        duration: 220
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: showMoveAnimation
        target: root
        property: "y"
        from: -root.height
        to: 0
        duration: 250
        easing.type: Easing.OutCubic
    }

    // 向上滑动透明度动画
    NumberAnimation {
        id: upOpacityAnimation
        target: root
        property: "opacity"
        from: 1
        to: 0
        duration: 250
        easing {type: Easing.OutQuad}  //
    }

    // 向上隐藏动画
    NumberAnimation {
        id: upMoveAnimation
        target: root
        property: "y"
        from: 0
        to: -root.height
        duration: 250
        easing {type: Easing.OutQuad}
    }

    function hide() {
        if (root.y !== -root.height) {
            upMoveAnimation.start()
            upOpacityAnimation.start()
        }
    }

    // Dlink快捷
    Label {
        width: 150
        height: 34
        anchors.left: parent.left
        anchors.leftMargin: 282
        anchors.top: parent.top
        anchors.topMargin: 106
        text: qsTr("DiLink 快捷")
        color: "#FFFFFF"
        font.pixelSize: 24
    }

    // 无线网络
    IconSwitch {
        id: wlanSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 277
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: Ui.controlCenterWLANStatus
        sourceOn: "qrc:/Images/ControlCenter/wlan_on.png"
        sourceOff: "qrc:/Images/ControlCenter/wlan_off.png"
        text: qsTr("无线网络")

        onSwitchStatusChanged: {
            Ui.controlCenterWLANStatus = switchStatus
        }
    }

    // 蓝牙
    IconSwitch {
        id: bluetoothSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 425
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: Ui.controlCenterBluetoothStatus
        sourceOn: "qrc:/Images/ControlCenter/bluetooth_on.png"
        sourceOff: "qrc:/Images/ControlCenter/bluetooth_off.png"
        text: qsTr("蓝牙")

        onSwitchStatusChanged: {
            Ui.controlCenterBluetoothStatus = switchStatus
        }
    }

    // 主题模式
    IconSwitch {
        id: themeModeSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 573
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/theme_mode_on.png"
        sourceOff: "qrc:/Images/ControlCenter/theme_mode_off.png"
        text: switchStatus ? qsTr("深色主题") : qsTr("浅色主题")
    }

    // 远程位置
    IconSwitch {
        id: remotePostionSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 277
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: Ui.controlCenterPositionStatus
        sourceOn: "qrc:/Images/ControlCenter/romote_postion_on.png"
        sourceOff: "qrc:/Images/ControlCenter/romote_postion_off.png"
        text: qsTr("远程位置")

        onSwitchStatusChanged: {
            Ui.controlCenterPositionStatus = switchStatus
        }
    }

    // 千里眼
    IconSwitch {
        id: clairvoyanceSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 425
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/clairvoyance_on.png"
        sourceOff: "qrc:/Images/ControlCenter/clairvoyance_off.png"
        text: qsTr("千里眼")
    }

    // 屏幕旋转
    IconSwitch {
        id: autoRotationSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 573
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: Ui.screenRotation === 90
        sourceOn: "qrc:/Images/ControlCenter/auto_rotation_on.png"
        sourceOff: "qrc:/Images/ControlCenter/auto_rotation_off.png"
        text: qsTr("屏幕旋转")

        onClicked: Ui.toggleScreenRotation()
    }

    Connections {
        target: Ui
        function onScreenRotationChanged() {
            autoRotationSwitch.switchStatus = Ui.screenRotation === 90
        }
    }


    //////////////////////////////////////////////////////
    // 整车控制
    Label {
        width: 150
        height: 34
        anchors.left: parent.left
        anchors.leftMargin: 733
        anchors.top: parent.top
        anchors.topMargin: 106
        text: qsTr("整车控制")
        color: "#FFFFFF"
        font.pixelSize: 24
    }

    // 抬头显示
    IconSwitch {
        id: hudSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 721
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/hud_on.png"
        sourceOff: "qrc:/Images/ControlCenter/hud_off.png"
        sourceWidth: 50
        sourceHeight: 40
        text: qsTr("抬头显示")
    }

    // 动态悬架
    IconSwitch {
        id: dynamicSuspensionSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 869
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/dynamic_suspension_on.png"
        sourceOff: "qrc:/Images/ControlCenter/dynamic_suspension_off.png"
        sourceWidth: 40
        sourceHeight: 40
        text: qsTr("动态悬架")
    }

    // 通风加热
    IconSwitch {
        id: ventilationSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 1017
        anchors.top: parent.top
        anchors.topMargin: 155
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/ventilation_on.png"
        sourceOff: "qrc:/Images/ControlCenter/ventilation_off.png"
        sourceWidth: 40
        sourceHeight: 40
        text: qsTr("通风加热")
    }

    // 电除霜
    IconSwitch {
        id: defrostSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 721
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/defrost_on.png"
        sourceOff: "qrc:/Images/ControlCenter/defrost_off.png"
        sourceWidth: 40
        sourceHeight: 40
        text: qsTr("电除霜")
    }

    // 天窗
    IconSwitch {
        id: windowSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 869
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/window_on.png"
        sourceOff: "qrc:/Images/ControlCenter/window_off.png"
        sourceWidth: 40
        sourceHeight: 40
        text: qsTr("天窗")
    }

    // ESP
    IconSwitch {
        id: espSwitch
        width: 113
        height: 113
        anchors.left: parent.left
        anchors.leftMargin: 1017
        anchors.top: parent.top
        anchors.topMargin: 302
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/esp_on.png"
        sourceOff: "qrc:/Images/ControlCenter/esp_off.png"
        sourceWidth: 50
        sourceHeight: 40
        text: qsTr("ESP")
    }

    //////////////////////////////////////////////////////
    // 音量
    Label {
        width: 150
        height: 34
        anchors.left: parent.left
        anchors.leftMargin: 282
        anchors.top: parent.top
        anchors.topMargin: 449
        text: qsTr("音量")
        color: "#FFFFFF"
        font.pixelSize: 24
    }

    // 媒体音量
    QuickSlider {
        id: mediaVolumeSlider
        width: 405
        height: 95
        anchors.left: parent.left
        anchors.leftMargin: 279
        anchors.top: parent.top
        anchors.topMargin: 496
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/volume.png"
        sourceOff: "qrc:/Images/ControlCenter/mute.png"
        sourceWidth: 36
        sourceHeight: 31
        textWidth: 100
        textHeight: sourceHeight
        text: qsTr("媒体音量")
        value: Ui.controlCenterMediaVolume
        minValue: 0
        maxValue: 10
        fontColor: "#0BC3C4"
        controlled: true

        onValueEdited: function(editedValue) {
            Ui.controlCenterMediaVolume = editedValue
        }
    }

    // 导航音量
    QuickSlider {
        id: navigationVolumeSlider
        width: 405
        height: 95
        anchors.left: parent.left
        anchors.leftMargin: 279
        anchors.top: parent.top
        anchors.topMargin: 626
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/volume.png"
        sourceOff: "qrc:/Images/ControlCenter/mute.png"
        sourceWidth: 36
        sourceHeight: 31
        textWidth: 100
        textHeight: sourceHeight
        text: qsTr("导航音量")
        value: 5
        minValue: 0
        maxValue: 10
        fontColor: "#0BC3C4"

        onValueChanged: {
        }
    }

    //////////////////////////////////////////////////////
    // 亮度
    Label {
        width: 150
        height: 34
        anchors.left: parent.left
        anchors.leftMargin: 733
        anchors.top: parent.top
        anchors.topMargin: 449
        text: qsTr("亮度")
        color: "#FFFFFF"
        font.pixelSize: 24
    }

    // 中控亮度
    QuickSlider {
        id: centerControlScreenSlider
        width: 405
        height: 95
        anchors.left: parent.left
        anchors.leftMargin: 725
        anchors.top: parent.top
        anchors.topMargin: 496
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/brightness.png"
        sourceOff: "qrc:/Images/ControlCenter/brightness_off.png"
        sourceWidth: 36
        sourceHeight: 31
        textWidth: 100
        textHeight: sourceHeight
        text: qsTr("中控亮度")
        value: 50
        fontColor: "#3874F2"
        autoMode: 1

        onValueChanged: {
        }
    }

    // 仪表亮度
    QuickSlider {
        id: meterSlider
        width: 405
        height: 95
        anchors.left: parent.left
        anchors.leftMargin: 725
        anchors.top: parent.top
        anchors.topMargin: 626
        switchStatus: false
        sourceOn: "qrc:/Images/ControlCenter/brightness.png"
        sourceOff: "qrc:/Images/ControlCenter/brightness_off.png"
        sourceWidth: 36
        sourceHeight: 31
        textWidth: 100
        textHeight: sourceHeight
        text: qsTr("仪表亮度")
        value: 50
        fontColor: "#3874F2"
        autoMode: 2

        onValueChanged: {
        }
    }

    // 指示条
    Rectangle {
        width: 127
        height: 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 811
        radius: height / 2
        color: "#CDFFFFFF"
    }
}
