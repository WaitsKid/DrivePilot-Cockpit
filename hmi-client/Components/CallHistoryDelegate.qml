import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    signal redialClicked()

    radius: 16
    color: historyMouse.containsMouse ? "#2B3545" : "#222B39"
    border.width: 1
    border.color: historyMouse.containsMouse ? "#4579B9FF" : "#1FFFFFFF"

    MouseArea {
        id: historyMouse
        anchors.fill: parent
        hoverEnabled: true
        onDoubleClicked: root.redialClicked()
    }

    Rectangle {
        id: directionBadge
        width: 48
        height: 48
        radius: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        color: "#26374A"

        Label {
            anchors.centerIn: parent
            text: model.direction === "incoming" ? "↙" : "↗"
            color: model.status.indexOf("未配置") >= 0 ? "#F59B9B" : "#69D39C"
            font.pixelSize: 25
            font.weight: Font.DemiBold
        }
    }

    Column {
        anchors.left: directionBadge.right
        anchors.leftMargin: 15
        anchors.right: redialButton.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 5

        Row {
            spacing: 12
            Label {
                text: model.displayName
                color: "#F4F7FB"
                font.pixelSize: 17
                font.weight: Font.DemiBold
            }
            Label {
                text: model.relativeTime
                color: "#8290A4"
                font.pixelSize: 13
            }
        }

        Label {
            text: model.phone + "  ·  " + model.status
            color: "#A9B5C8"
            font.pixelSize: 14
            elide: Text.ElideRight
            width: parent.width
        }
    }

    Button {
        id: redialButton
        width: 48
        height: 48
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        hoverEnabled: false
        contentItem: Label {
            text: "☎"
            color: "white"
            font.pixelSize: 22
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 17
            color: redialButton.down ? "#2F8C61" : "#3EAA76"
        }
        onClicked: root.redialClicked()
    }
}
