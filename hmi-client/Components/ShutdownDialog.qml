import QtQuick
import QtQuick.Controls

Popup {
    id: root

    property real uiScale: 1.0
    readonly property real dialogScale: Math.max(0.72, Math.min(1.15, uiScale))
    signal confirmed

    width: 520 * dialogScale
    height: 286 * dialogScale
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    modal: true
    focus: true
    dim: true
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: "#86060A10"
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
            NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: 180; easing.type: Easing.OutCubic }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140 }
            NumberAnimation { property: "scale"; from: 1; to: 0.97; duration: 140; easing.type: Easing.InCubic }
        }
    }

    background: Rectangle {
        radius: 28 * root.dialogScale
        color: "#F21C2432"
        border.width: Math.max(1, root.dialogScale)
        border.color: "#486FE9FF"
    }

    contentItem: Item {
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 34 * root.dialogScale
            text: qsTr("关闭智能座舱")
            color: "#FFFFFF"
            font.pixelSize: 28 * root.dialogScale
            font.weight: Font.DemiBold
        }

        Label {
            width: parent.width - 72 * root.dialogScale
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 93 * root.dialogScale
            text: qsTr("确认退出当前 HMI 演示程序吗？\n未完成的设置将自动保存。")
            color: "#BFFFFFFF"
            font.pixelSize: 17 * root.dialogScale
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.35
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 28 * root.dialogScale
            spacing: 18 * root.dialogScale

            Button {
                width: 188 * root.dialogScale
                height: 54 * root.dialogScale
                hoverEnabled: false
                contentItem: Label {
                    text: qsTr("取消")
                    color: "#EFFFFFFF"
                    font.pixelSize: 18 * root.dialogScale
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 27 * root.dialogScale
                    color: parent.down ? "#3B4555" : "#2B3544"
                    border.width: 1
                    border.color: "#30FFFFFF"
                }
                onClicked: root.close()
            }

            Button {
                width: 188 * root.dialogScale
                height: 54 * root.dialogScale
                hoverEnabled: false
                contentItem: Label {
                    text: qsTr("确认关闭")
                    color: "#FFFFFF"
                    font.pixelSize: 18 * root.dialogScale
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 27 * root.dialogScale
                    color: parent.down ? "#D44A4A" : "#EE5B5B"
                }
                onClicked: {
                    root.close()
                    root.confirmed()
                }
            }
        }
    }
}
