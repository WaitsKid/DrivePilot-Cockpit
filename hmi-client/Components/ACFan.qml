import QtQuick
import QtQuick.Controls
import DrivePilot

// 空调风量弹窗。
// Popup 会显示在 Window 的 Overlay 中，不会继承 designCanvas.scale，
// 因此这里根据窗口尺寸重新计算等比缩放和留边位置。
Popup {
    id: root

    property color backgroundColor: "#222A3B"
    property int realLevel: 5
    property int fanLevel: Math.round(fanSlider.value)
    property int delay: 3000

    readonly property real designCanvasWidth: 1521
    readonly property real designCanvasHeight: 856
    readonly property real designPopupX: 431
    readonly property real designPopupY: 617
    readonly property real designPopupWidth: 723
    readonly property real designPopupHeight: 71

    readonly property real overlayWidth: parent ? parent.width : designCanvasWidth
    readonly property real overlayHeight: parent ? parent.height : designCanvasHeight
    readonly property bool portraitOrientation: Ui.screenRotation === 90
    readonly property real referenceWidth: portraitOrientation ? designCanvasHeight : designCanvasWidth
    readonly property real referenceHeight: portraitOrientation ? designCanvasWidth : designCanvasHeight
    readonly property real uiScale: Math.max(
                                        0.01,
                                        Math.min(overlayWidth / referenceWidth,
                                                 overlayHeight / referenceHeight))
    readonly property real horizontalLetterbox: Math.max(
                                                    0,
                                                    (overlayWidth - referenceWidth * uiScale) / 2)
    readonly property real verticalLetterbox: Math.max(
                                                  0,
                                                  (overlayHeight - referenceHeight * uiScale) / 2)

    width: designPopupWidth * uiScale
    height: designPopupHeight * uiScale
    x: portraitOrientation
       ? Math.round((overlayWidth - width) / 2)
       : horizontalLetterbox + designPopupX * uiScale
    y: portraitOrientation
       ? overlayHeight - verticalLetterbox - height - 105 * uiScale
       : verticalLetterbox + designPopupY * uiScale
    padding: 0

    modal: false
    focus: false
    dim: false
    closePolicy: Popup.CloseOnPressOutside
    parent: Overlay.overlay

    onOpened: closeTimer.restart()

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 220
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                property: "scale"
                from: 0.96
                to: 1
                duration: 220
                easing.type: Easing.OutQuad
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 180
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                property: "scale"
                from: 1
                to: 0.98
                duration: 180
                easing.type: Easing.InQuad
            }
        }
    }

    Timer {
        id: closeTimer
        interval: root.delay
        repeat: false
        onTriggered: root.close()
    }

    background: Rectangle {
        radius: 35 * root.uiScale
        color: root.backgroundColor
        opacity: 0.92
        border.width: Math.max(1, root.uiScale)
        border.color: "#24FFFFFF"
    }

    contentItem: Item {
        ColorSlider {
            id: fanSlider
            width: 535 * root.uiScale
            height: 19 * root.uiScale
            anchors.centerIn: parent
            minValue: 0
            maxValue: 10
            value: Ui.acFanLevel
            cornerRadius: 14 * root.uiScale

            onValueModified: {
                closeTimer.restart()
                const roundedValue = Math.round(value)
                if (Ui.acFanLevel !== roundedValue)
                    Ui.acFanLevel = roundedValue
            }
        }

        Button {
            id: subButton
            width: 50 * root.uiScale
            height: 50 * root.uiScale
            anchors.left: parent.left
            anchors.leftMargin: 39 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: false
            autoRepeat: true
            autoRepeatInterval: 200

            background: Image {
                width: 21 * root.uiScale
                height: 21 * root.uiScale
                anchors.centerIn: parent
                source: "qrc:/Images/ACFan/fan_sub.png"
                fillMode: Image.PreserveAspectFit
                opacity: parent.down ? 0.6 : 1
            }

            onPressed: closeTimer.restart()
            onClicked: {
                closeTimer.restart()
                if (Ui.acFanLevel > 0)
                    Ui.acFanLevel -= 1
            }
        }

        Button {
            id: addButton
            width: 50 * root.uiScale
            height: 50 * root.uiScale
            anchors.right: parent.right
            anchors.rightMargin: 39 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: false
            autoRepeat: true
            autoRepeatInterval: 200

            background: Image {
                width: 33 * root.uiScale
                height: 33 * root.uiScale
                anchors.centerIn: parent
                source: "qrc:/Images/ACFan/fan_add.png"
                fillMode: Image.PreserveAspectFit
                opacity: parent.down ? 0.6 : 1
            }

            onPressed: closeTimer.restart()
            onClicked: {
                closeTimer.restart()
                if (Ui.acFanLevel < 10)
                    Ui.acFanLevel += 1
            }
        }
    }
}
