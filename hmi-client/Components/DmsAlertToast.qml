import QtQuick
import QtQuick.Controls
import DrivePilot

Popup {
    id: root

    property real uiScale: 1.0
    property real panelRightMargin: 24 * uiScale
    property real panelTopMargin: 62 * uiScale
    property int severityLevel: 2
    property string alertMessage: ""

    width: 450 * uiScale
    height: 122 * uiScale
    x: parent ? Math.max(0, parent.width - width - panelRightMargin) : 0
    y: panelTopMargin
    padding: 0
    modal: false
    focus: false
    dim: false
    z: 1500
    closePolicy: Popup.NoAutoClose

    readonly property color accentColor: severityLevel >= 3 ? "#FF493B" : "#F3D33B"
    readonly property string titleText: severityLevel >= 3
                                        ? qsTr("严重疲劳警告")
                                        : qsTr("驾驶状态提醒")

    function showAlert(message, level) {
        alertMessage = message
        severityLevel = level
        if (!opened)
            open()
        hideTimer.interval = level >= 3 ? 12000 : 9000
        hideTimer.restart()
    }

    background: Rectangle {
        id: alertBackground
        radius: 20 * root.uiScale
        color: "#F218202B"
        border.width: Math.max(1, 2 * root.uiScale)
        border.color: root.accentColor

        Rectangle {
            width: 6 * root.uiScale
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: width / 2
            color: root.accentColor
        }
    }

    contentItem: Item {
        Image {
            width: 50 * root.uiScale
            height: 50 * root.uiScale
            anchors.left: parent.left
            anchors.leftMargin: 23 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            source: root.severityLevel >= 3
                    ? "qrc:/Images/Dms/severe_fatigue.png"
                    : "qrc:/Images/Dms/slight_fatigue.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Label {
            id: titleLabel
            anchors.left: parent.left
            anchors.leftMargin: 88 * root.uiScale
            anchors.top: parent.top
            anchors.topMargin: 18 * root.uiScale
            text: root.titleText
            color: root.accentColor
            font.pixelSize: 18 * root.uiScale
            font.bold: true
        }

        Label {
            anchors.left: titleLabel.left
            anchors.right: closeButton.left
            anchors.rightMargin: 12 * root.uiScale
            anchors.top: titleLabel.bottom
            anchors.topMargin: 7 * root.uiScale
            text: root.alertMessage
            color: "#FFFFFF"
            font.pixelSize: 15 * root.uiScale
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Label {
            anchors.left: titleLabel.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12 * root.uiScale
            text: qsTr("闭眼 %1 s · PERCLOS %2% · 哈欠 %3 次")
                    .arg((DmsSystem.closedDurationMs / 1000.0).toFixed(1))
                    .arg(Math.round(DmsSystem.perclos * 100))
                    .arg(DmsSystem.yawnCountWindow)
            color: "#93A8B8"
            font.pixelSize: 12 * root.uiScale
        }

        Button {
            id: closeButton
            width: 36 * root.uiScale
            height: 36 * root.uiScale
            anchors.right: parent.right
            anchors.rightMargin: 10 * root.uiScale
            anchors.top: parent.top
            anchors.topMargin: 8 * root.uiScale
            hoverEnabled: true

            contentItem: Label {
                text: "×"
                color: "#DFFFFFFF"
                font.pixelSize: 24 * root.uiScale
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 10 * root.uiScale
                color: closeButton.down
                       ? "#4DFFFFFF"
                       : (closeButton.hovered ? "#24FFFFFF" : "transparent")
            }

            onClicked: root.close()
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 190
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "x"
                from: root.parent ? root.parent.width : root.x + root.width
                to: root.parent ? root.parent.width - root.width - root.panelRightMargin : root.x
                duration: 240
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 160
            }
            NumberAnimation {
                property: "scale"
                from: 1
                to: 0.98
                duration: 160
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 9000
        repeat: false
        onTriggered: root.close()
    }
}
