import QtQuick
import QtQuick.Controls

Item {
    id: root

    property url iconSource
    property string appName
    property string categoryName
    property bool favorite: false
    property int launchCount: 0
    property bool available: false

    signal clicked()
    signal favoriteClicked()

    width: 150
    height: 158
    scale: tileMouse.pressed ? 0.96 : (tileMouse.containsMouse ? 1.03 : 1.0)

    Behavior on scale {
        NumberAnimation {
            duration: 120
            easing.type: Easing.OutQuad
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: tileMouse.containsMouse ? "#18FFFFFF" : "transparent"
        border.width: tileMouse.containsMouse ? 1 : 0
        border.color: "#2EFFFFFF"

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    Image {
        id: iconImage
        width: 90
        height: 90
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 9
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }

    Rectangle {
        width: 52
        height: 22
        anchors.left: iconImage.left
        anchors.bottom: iconImage.bottom
        anchors.leftMargin: 4
        anchors.bottomMargin: 4
        radius: 11
        color: "#B3192230"
        visible: root.available

        Label {
            anchors.centerIn: parent
            text: qsTr("内置")
            color: "#EFFFFFFF"
            font.pixelSize: 12
        }
    }

    Button {
        id: favoriteButton
        width: 34
        height: 34
        anchors.right: iconImage.right
        anchors.top: iconImage.top
        anchors.rightMargin: -7
        anchors.topMargin: -7
        hoverEnabled: false
        z: 3

        contentItem: Label {
            text: root.favorite ? "★" : "☆"
            color: root.favorite ? "#FFD36A" : "#D8FFFFFF"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: width / 2
            color: parent.down ? "#E62B3443" : "#B51B2432"
            border.width: 1
            border.color: "#28FFFFFF"
        }

        onClicked: root.favoriteClicked()
    }

    Label {
        id: nameLabel
        width: parent.width
        height: 29
        anchors.top: iconImage.bottom
        anchors.topMargin: 8
        text: root.appName
        color: "#FFFFFF"
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Label {
        width: parent.width
        height: 18
        anchors.top: nameLabel.bottom
        text: root.launchCount > 0
              ? qsTr("%1 · 使用 %2 次").arg(root.categoryName).arg(root.launchCount)
              : root.categoryName
        color: "#8FFFFFFF"
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    MouseArea {
        id: tileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: 1
        onClicked: root.clicked()
    }
}
