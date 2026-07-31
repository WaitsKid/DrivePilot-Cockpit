import QtQuick
import QtQuick.Controls

Button {
    id: root
    property color keyColor: "#253247"
    property color accentColor: "#5C7CFF"
    property bool accent: false

    width: 92
    height: 54
    hoverEnabled: false

    contentItem: Label {
        text: root.text
        color: "#FFFFFF"
        font.pixelSize: 17
        font.weight: root.accent ? Font.DemiBold : Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 16
        color: root.down ? "#4A5F7E" : (root.accent ? root.accentColor : root.keyColor)
        border.width: 1
        border.color: root.accent ? "#70A7F5" : "#20FFFFFF"
    }
}
