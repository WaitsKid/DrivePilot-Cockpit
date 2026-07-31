import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string videoTitle: ""
    property string subtitle: ""
    property string categoryName: ""
    property string durationText: ""
    property url posterSource: ""
    property color accentColor: "#5C7CFF"
    property bool favorite: false
    property bool current: false
    property bool available: true
    property real resumeProgress: 0.0

    signal clicked()
    signal favoriteClicked()
    signal restartClicked()

    width: 402
    height: 112

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: root.current ? "#334B70" : (mouseArea.containsMouse ? "#2A3444" : "#202936")
        border.width: root.current ? 2 : 1
        border.color: root.current ? root.accentColor : "#20FFFFFF"

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    Rectangle {
        id: posterFrame
        width: 154
        height: 88
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        radius: 12
        color: "#101722"
        clip: true

        Image {
            anchors.fill: parent
            source: root.posterSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            visible: status === Image.Ready
        }

        Rectangle {
            anchors.fill: parent
            visible: root.posterSource.toString().length === 0
            gradient: Gradient {
                GradientStop { position: 0.0; color: root.accentColor }
                GradientStop { position: 1.0; color: "#111827" }
            }
        }

        Rectangle {
            width: 38
            height: 38
            anchors.centerIn: parent
            radius: 19
            color: "#B3121720"
            border.width: 1
            border.color: "#52FFFFFF"

            Label {
                anchors.centerIn: parent
                text: root.current ? "▶" : "▷"
                color: "#FFFFFF"
                font.pixelSize: 18
                leftPadding: 2
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: 7
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 7
            width: durationLabel.implicitWidth + 12
            height: 22
            radius: 7
            color: "#B30A0E15"

            Label {
                id: durationLabel
                anchors.centerIn: parent
                text: root.durationText
                color: "#FFFFFF"
                font.pixelSize: 11
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 3
            color: "#22FFFFFF"

            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, root.resumeProgress))
                height: parent.height
                color: root.accentColor
            }
        }
    }

    Column {
        anchors.left: posterFrame.right
        anchors.leftMargin: 14
        anchors.right: favoriteButton.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Label {
            width: parent.width
            text: root.videoTitle
            color: root.available ? "#FFFFFF" : "#72FFFFFF"
            font.pixelSize: 17
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: root.subtitle
            color: "#9FFFFFFF"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Row {
            spacing: 8

            Rectangle {
                width: categoryLabel.implicitWidth + 14
                height: 22
                radius: 11
                color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.22)

                Label {
                    id: categoryLabel
                    anchors.centerIn: parent
                    text: root.categoryName
                    color: "#EFFFFFFF"
                    font.pixelSize: 11
                }
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: root.resumeProgress > 0.02 ? qsTr("继续播放") : qsTr("未观看")
                color: root.resumeProgress > 0.02 ? root.accentColor : "#6FFFFFFF"
                font.pixelSize: 11
            }
        }
    }

    Button {
        id: favoriteButton
        width: 42
        height: 42
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 8
        hoverEnabled: false

        contentItem: Label {
            text: root.favorite ? "★" : "☆"
            color: root.favorite ? "#FFD66B" : "#9FFFFFFF"
            font.pixelSize: 22
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 14
            color: favoriteButton.down ? "#30FFFFFF" : "transparent"
        }

        onClicked: root.favoriteClicked()
    }

    Button {
        width: 42
        height: 34
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        visible: root.resumeProgress > 0.02
        hoverEnabled: false

        contentItem: Label {
            text: "↺"
            color: "#BFFFFFFF"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 12
            color: parent.down ? "#30FFFFFF" : "transparent"
        }

        onClicked: root.restartClicked()
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.rightMargin: 52
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
