import QtQuick
import QtQuick.Controls

ScrollBar {
    id: root

    padding: 0

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        radius: 3
        color: root.pressed ? "#62B6FF" : (root.hovered ? "#8FFFFFFF" : "#4FFFFFFF")
    }

    background: Rectangle {
        color: "transparent"
    }
}
