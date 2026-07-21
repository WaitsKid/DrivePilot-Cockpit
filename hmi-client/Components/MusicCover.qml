import QtQuick

Item {
    id: root

    property color primaryColor: "#5C7CFF"
    property color secondaryColor: "#B24DFF"
    property string title: "N"
    property int variant: 0
    property bool playing: false
    property int cornerRadius: 28

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        clip: true
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: root.primaryColor }
            GradientStop { position: 1.0; color: root.secondaryColor }
        }

        Repeater {
            model: 7

            Rectangle {
                required property int index
                width: parent.width * 1.15
                height: Math.max(2, parent.height * 0.012)
                x: -parent.width * 0.08
                y: parent.height * (0.12 + index * 0.12)
                rotation: root.variant === 1 ? -24 : (root.variant === 2 ? 24 : -12)
                color: index % 2 === 0 ? "#20FFFFFF" : "#0D000000"
            }
        }

        Rectangle {
            width: parent.width * 0.34
            height: width
            radius: width / 2
            anchors.right: parent.right
            anchors.rightMargin: -width * 0.2
            anchors.top: parent.top
            anchors.topMargin: -height * 0.18
            color: "#28FFFFFF"
        }
    }

    Item {
        id: rotatingDisc
        width: Math.min(root.width, root.height) * 0.66
        height: width
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "#D9141925"
            border.width: Math.max(1, width * 0.015)
            border.color: "#35FFFFFF"
        }

        Repeater {
            model: 5

            Rectangle {
                required property int index
                width: rotatingDisc.width * (0.9 - index * 0.13)
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "transparent"
                border.width: Math.max(1, rotatingDisc.width * 0.008)
                border.color: index % 2 === 0 ? "#22FFFFFF" : "#18000000"
            }
        }

        Rectangle {
            width: parent.width * 0.38
            height: width
            radius: width / 2
            anchors.centerIn: parent
            color: root.secondaryColor
            border.width: Math.max(1, width * 0.05)
            border.color: "#50FFFFFF"

            Text {
                anchors.centerIn: parent
                text: root.title.length > 0 ? root.title.charAt(0).toUpperCase() : "M"
                color: "#F5FFFFFF"
                font.pixelSize: parent.width * 0.42
                font.bold: true
            }
        }

        Rectangle {
            width: parent.width * 0.06
            height: width
            radius: width / 2
            anchors.centerIn: parent
            color: "#E8F2F8"
        }

        RotationAnimation on rotation {
            from: 0
            to: 360
            duration: 16000
            loops: Animation.Infinite
            running: root.playing
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.width: 1
        border.color: "#28FFFFFF"
    }
}
