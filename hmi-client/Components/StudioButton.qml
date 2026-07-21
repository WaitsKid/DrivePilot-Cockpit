import QtQuick
import QtQuick.Controls

Button {
    id: root

    property string glyph: ""
    property bool selected: false
    property bool danger: false
    property bool compact: false
    property bool touch: false

    implicitWidth: compact ? 82 : 126
    implicitHeight: compact ? 36 : 42
    hoverEnabled: !root.touch

    contentItem: Row {
        anchors.centerIn: parent
        spacing: root.glyph.length > 0 && root.text.length > 0 ? 7 : 0

        Label {
            visible: root.glyph.length > 0
            text: root.glyph
            color: root.enabled ? "#FFFFFF" : "#58FFFFFF"
            font.pixelSize: root.compact ? 15 : 18
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            visible: root.text.length > 0
            text: root.text
            color: root.enabled ? "#FFFFFF" : "#58FFFFFF"
            font.pixelSize: root.compact ? 12 : 14
            font.weight: root.selected ? Font.DemiBold : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    background: Rectangle {
        radius: root.compact ? 11 : 14
        color: {
            if (!root.enabled)
                return "#241F2A39"
            if (root.down)
                return root.danger ? "#AD354A" : "#42698F"
            if (root.selected)
                return "#355D82"
            if (!root.touch && root.hovered)
                return "#31465F"
            return root.danger ? "#5A2C39" : "#243247"
        }
        border.width: root.selected ? 2 : 1
        border.color: root.selected ? "#72C4FF" : "#26FFFFFF"
    }
}
