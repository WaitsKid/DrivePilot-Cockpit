import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string value: "--"
    property string detail: ""
    property string iconText: "•"
    property color accentColor: "#79AFFF"

    radius: 18
    color: "#18212F"
    border.width: 1
    border.color: "#22FFFFFF"

    Rectangle {
        width: 38
        height: 38
        radius: 12
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.top: parent.top
        anchors.topMargin: 15
        color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.17)

        Text {
            anchors.centerIn: parent
            text: root.iconText
            color: root.accentColor
            font.pixelSize: 21
            font.bold: true
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 66
        anchors.top: parent.top
        anchors.topMargin: 16
        text: root.title
        color: "#8FFFFFFF"
        font.pixelSize: 14
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 66
        anchors.top: parent.top
        anchors.topMargin: 38
        text: root.value
        color: "#FFFFFF"
        font.pixelSize: 22
        font.weight: Font.DemiBold
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 11
        text: root.detail
        color: "#72FFFFFF"
        font.pixelSize: 12
        elide: Text.ElideRight
    }
}
