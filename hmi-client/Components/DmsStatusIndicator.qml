import QtQuick
import QtQuick.Controls
import DrivePilot

Item {
    id: root

    property real uiScale: 1.0

    implicitWidth: 48 * uiScale
    implicitHeight: 40 * uiScale
    width: implicitWidth
    height: implicitHeight

    readonly property string statusIcon: {
        switch (DmsSystem.fatigueLevel) {
        case 0: return "qrc:/Images/Dms/energetic.png"
        case 2: return "qrc:/Images/Dms/slight_fatigue.png"
        case 3: return "qrc:/Images/Dms/severe_fatigue.png"
        default: return "qrc:/Images/Dms/normal.png"
        }
    }

    readonly property color connectionColor: {
        if (!DmsSystem.serviceAvailable)
            return "#8B95A3"
        if (!DmsSystem.serviceRunning || !DmsSystem.cameraAvailable)
            return "#F1C75B"
        if (!DmsSystem.faceDetected)
            return "#77B7F2"
        return "#4DDB96"
    }

    readonly property string detailText: {
        if (!DmsSystem.serviceAvailable)
            return qsTr("疲劳监测服务离线\n%1").arg(DmsSystem.endpoint)
        if (!DmsSystem.modelsReady)
            return qsTr("疲劳监测模型尚未就绪")
        if (!DmsSystem.serviceRunning)
            return qsTr("疲劳监测已停止，点击图标尝试启动")
        if (!DmsSystem.cameraAvailable)
            return qsTr("摄像头不可用：%1").arg(DmsSystem.lastError)
        if (!DmsSystem.faceDetected)
            return qsTr("后台监测运行中 · 未检测到驾驶员")

        const voiceState = !DmsSystem.voiceEnabled
                ? qsTr("语音已关闭")
                : (DmsSystem.voiceAvailable
                   ? qsTr("语音提醒已就绪")
                   : qsTr("语音提醒不可用"))

        return qsTr("%1\n闭眼 %2% · PERCLOS %3% · 哈欠 %4 次\n推理 %5 ms · %6 FPS\n%7")
                .arg(DmsSystem.statusText)
                .arg(Math.round(DmsSystem.closedProbability * 100))
                .arg(Math.round(DmsSystem.perclos * 100))
                .arg(DmsSystem.yawnCountWindow)
                .arg(DmsSystem.inferenceMs.toFixed(1))
                .arg(DmsSystem.processedFps.toFixed(1))
                .arg(voiceState)
    }

    Rectangle {
        anchors.fill: parent
        radius: 12 * root.uiScale
        color: hoverArea.containsMouse ? "#2D314153" : "transparent"
        border.width: hoverArea.containsMouse ? Math.max(1, root.uiScale) : 0
        border.color: "#30FFFFFF"

        Image {
            id: iconImage
            width: 31 * root.uiScale
            height: 31 * root.uiScale
            anchors.centerIn: parent
            source: root.statusIcon
            fillMode: Image.PreserveAspectFit
            smooth: true
            opacity: DmsSystem.serviceAvailable ? 1.0 : 0.38
        }

        Rectangle {
            width: 9 * root.uiScale
            height: 9 * root.uiScale
            radius: width / 2
            anchors.right: parent.right
            anchors.rightMargin: 3 * root.uiScale
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 3 * root.uiScale
            color: root.connectionColor
            border.width: Math.max(1, root.uiScale)
            border.color: "#D10A111A"
        }
    }

    SequentialAnimation {
        running: DmsSystem.fatigueLevel === 3 && DmsSystem.serviceAvailable
        loops: Animation.Infinite

        NumberAnimation {
            target: iconImage
            property: "scale"
            from: 1.0
            to: 1.13
            duration: 420
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: iconImage
            property: "scale"
            from: 1.13
            to: 1.0
            duration: 420
            easing.type: Easing.InOutQuad
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton

        onClicked: {
            if (!DmsSystem.serviceAvailable) {
                DmsSystem.refreshNow()
            } else if (!DmsSystem.serviceRunning) {
                DmsSystem.startMonitoring()
            } else {
                DmsSystem.refreshNow()
            }
        }

        ToolTip.visible: containsMouse
        ToolTip.delay: 260
        ToolTip.timeout: 8000
        ToolTip.text: root.detailText
    }
}
