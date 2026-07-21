import QtQuick
import QtQuick.Controls

Button {
    id: root

    property string iconText: "▶"
    property bool emphasized: false
    property color accentColor: "#5C7CFF"

    width: emphasized ? 72 : 54
    height: width
    hoverEnabled: false

    contentItem: Text {
        text: root.iconText
        color: "#F4FFFFFF"
        font.pixelSize: root.emphasized ? 29 : 23
        font.bold: root.emphasized
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: width / 2
        color: root.emphasized
               ? (root.down ? Qt.darker(root.accentColor, 1.25) : root.accentColor)
               : (root.down ? "#34FFFFFF" : (root.hovered ? "#24FFFFFF" : "#18FFFFFF"))
        border.width: root.emphasized ? 0 : 1
        border.color: "#2FFFFFFF"

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }
}
