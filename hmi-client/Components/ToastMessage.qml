import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root

    property alias text: messageLabel.text
    property real uiScale: 1.0
    property real toastBottomInset: 200 * uiScale

    width: Math.max(300 * uiScale,
                    Math.min(780 * uiScale,
                             messageLabel.implicitContentWidth + 72 * uiScale))
    height: 62 * uiScale
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round(parent.height - height - toastBottomInset) : 0
    padding: 0
    modal: false
    focus: false
    dim: false
    z: 1000
    closePolicy: Popup.NoAutoClose

    function show(message) {
        messageLabel.text = message

        if (!opened)
            open()

        hideTimer.restart()
    }

    background: Rectangle {
        radius: 16 * root.uiScale
        color: "#E6222835"
        border.width: Math.max(1, root.uiScale)
        border.color: "#406FE9FF"
    }

    contentItem: Label {
        id: messageLabel
        leftPadding: 28 * root.uiScale
        rightPadding: 28 * root.uiScale
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#FFFFFF"
        font.pixelSize: 20 * root.uiScale
        elide: Text.ElideRight
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 180
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"
                from: 0.96
                to: 1
                duration: 180
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
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "scale"
                from: 1
                to: 0.98
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 1800
        repeat: false
        onTriggered: root.close()
    }
}