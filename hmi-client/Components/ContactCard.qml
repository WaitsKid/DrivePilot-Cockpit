import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    signal callClicked()
    signal favoriteClicked()
    signal editClicked()

    radius: 18
    color: contactMouse.containsMouse ? "#2B3545" : "#222B39"
    border.width: 1
    border.color: contactMouse.containsMouse ? "#4579B9FF" : "#20FFFFFF"

    Behavior on color { ColorAnimation { duration: 120 } }

    MouseArea {
        id: contactMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: root.callClicked()
    }

    Rectangle {
        id: avatar
        width: 58
        height: 58
        radius: 29
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        color: model.avatarColor

        Label {
            anchors.centerIn: parent
            text: model.initials
            color: "white"
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }
    }

    Column {
        anchors.left: avatar.right
        anchors.leftMargin: 16
        anchors.right: favoriteButton.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Label {
            width: parent.width
            text: model.name
            color: "#F4F7FB"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: model.phone
            color: "#A9B5C8"
            font.pixelSize: 15
            font.family: "Consolas"
            elide: Text.ElideRight
        }
    }

    Button {
        id: favoriteButton
        width: 42
        height: 42
        anchors.right: editButton.left
        anchors.rightMargin: 7
        anchors.verticalCenter: parent.verticalCenter
        hoverEnabled: false
        contentItem: Label {
            text: model.favorite ? "★" : "☆"
            color: model.favorite ? "#FFD36A" : "#90A0B6"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 14
            color: favoriteButton.down ? "#314053" : "#1B2330"
        }
        onClicked: root.favoriteClicked()
    }

    Button {
        id: editButton
        width: 42
        height: 42
        anchors.right: callButton.left
        anchors.rightMargin: 7
        anchors.verticalCenter: parent.verticalCenter
        hoverEnabled: false
        contentItem: Label {
            text: "✎"
            color: "#B9C7DA"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 14
            color: editButton.down ? "#314053" : "#1B2330"
        }
        onClicked: root.editClicked()
    }

    Button {
        id: callButton
        width: 50
        height: 50
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        hoverEnabled: false
        contentItem: Label {
            text: "☎"
            color: "white"
            font.pixelSize: 23
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 18
            color: callButton.down ? "#2F8C61" : "#3EAA76"
        }
        onClicked: root.callClicked()
    }
}
