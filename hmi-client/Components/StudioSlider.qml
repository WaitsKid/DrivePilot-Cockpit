import QtQuick
import QtQuick.Controls

Slider {
    id: root

    implicitWidth: 140
    implicitHeight: 26

    padding: 0

    background: Rectangle {
        x: 8
        y: (root.height - height) / 2
        width: root.availableWidth - 16
        height: 4
        radius: 2
        color: "#243247"
        border.width: 1
        border.color: "#18FFFFFF"

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: "#62B6FF"
        }
    }

    handle: Rectangle {
        x: root.visualPosition * (root.availableWidth - width)
        y: (root.height - height) / 2
        width: 16
        height: 16
        radius: 8
        color: root.pressed ? "#D6EFFF" : "#FFFFFF"

        Behavior on color { ColorAnimation { duration: 100 } }

        Rectangle {
            anchors.centerIn: parent
            width: 5
            height: 5
            radius: 2.5
            color: "#62B6FF"
            opacity: root.pressed ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
    }
}
