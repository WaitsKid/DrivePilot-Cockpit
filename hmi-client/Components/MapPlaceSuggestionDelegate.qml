import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string placeName: ""
    property string district: ""
    property string address: ""
    property string distanceText: ""
    property int relevanceScore: 0
    property bool highlighted: false

    signal clicked

    implicitHeight: 68

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: root.highlighted || hoverHandler.hovered ? "#314B68" : "transparent"
        border.width: root.highlighted ? 1 : 0
        border.color: "#628FC2"

        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Rectangle {
        width: 34
        height: 34
        radius: 17
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#36E7EA" }
            GradientStop { position: 1.0; color: "#287FEA" }
        }

        Label {
            anchors.centerIn: parent
            text: "⌖"
            color: "#FFFFFF"
            font.pixelSize: 18
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: 58
        anchors.right: distanceLabel.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Label {
            width: parent.width
            text: root.placeName
            color: "#FFFFFF"
            font.pixelSize: 16
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: {
                const detail = root.address.length > 0 ? root.address : root.district
                return detail.length > 0 ? detail : qsTr("地址信息暂缺")
            }
            color: "#9FB5CB"
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    Label {
        id: distanceLabel
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: root.distanceText
        color: "#70C9FF"
        font.pixelSize: 12
        font.bold: true
    }

    HoverHandler { id: hoverHandler }

    TapHandler {
        onTapped: root.clicked()
    }
}
