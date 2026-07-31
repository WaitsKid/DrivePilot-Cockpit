import QtQuick
import QtQuick.Controls

Button {
    id: root

    property string digit: ""
    property string letters: ""
    signal dialClicked(string value)

    width: 92
    height: 68
    hoverEnabled: false

    contentItem: Column {
        anchors.centerIn: parent
        spacing: 1

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.digit
            color: "#F5F8FC"
            font.pixelSize: 27
            font.weight: Font.DemiBold
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.letters
            color: "#7F8EA4"
            font.pixelSize: 10
            font.letterSpacing: 1.1
            visible: text.length > 0
        }
    }

    background: Rectangle {
        radius: 22
        color: root.down ? "#3B4D64" : "#283342"
        border.width: 1
        border.color: root.down ? "#6EA6F3" : "#24FFFFFF"
    }

    onClicked: root.dialClicked(root.digit)
}
