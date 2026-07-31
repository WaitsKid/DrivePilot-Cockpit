import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string title: ""
    property string displayValue: "--"
    property string unit: ""
    property string detail: ""
    property color accentColor: "#4DE6C8"

    radius: 18
    color: "#241F2B3C"
    border.width: 1
    border.color: "#24FFFFFF"

    Rectangle {
        width: 4
        height: 44
        radius: 2
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        color: root.accentColor
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: 13
        text: root.title
        color: "#8FFFFFFF"
        font.pixelSize: 14
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: 35
        spacing: 5

        Label {
            text: root.displayValue
            color: "#FFFFFF"
            font.pixelSize: 26
            font.bold: true
        }

        Label {
            text: root.unit
            color: "#AFFFFFFF"
            font.pixelSize: 13
        }
    }

    Label {
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        text: root.detail
        color: root.accentColor
        font.pixelSize: 12
    }
}
