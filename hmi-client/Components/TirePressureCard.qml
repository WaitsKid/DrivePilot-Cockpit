import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string positionText: "左前"
    property real pressure: 2.4
    readonly property bool warning: pressure < 2.0 || pressure > 3.0

    radius: 16
    color: warning ? "#35FF5D67" : "#211E2A38"
    border.width: 1
    border.color: warning ? "#CFFF6570" : "#22FFFFFF"

    Rectangle {
        width: 18
        height: 47
        radius: 8
        anchors.left: parent.left
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        color: warning ? "#FF6570" : "#687D91"

        Rectangle {
            width: 8
            height: 31
            radius: 4
            anchors.centerIn: parent
            color: "#17202C"
        }
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.top: parent.top
        anchors.topMargin: 13
        text: root.positionText
        color: "#90FFFFFF"
        font.pixelSize: 14
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        spacing: 4

        Label {
            text: root.pressure.toFixed(2)
            color: root.warning ? "#FF737C" : "#FFFFFF"
            font.pixelSize: 23
            font.bold: true
        }

        Label {
            text: "bar"
            color: "#8FFFFFFF"
            font.pixelSize: 12
        }
    }

    Label {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: root.warning ? qsTr("异常") : qsTr("正常")
        color: root.warning ? "#FF737C" : "#50E3C2"
        font.pixelSize: 12
        font.bold: true
    }
}
