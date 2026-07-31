import QtQuick

Item {
    id: root

    property string cityName: ""
    property string detailText: ""
    property string sourceText: ""
    property string levelText: ""
    property string adcodeText: ""
    property bool highlighted: false

    signal chosen()

    width: ListView.view ? ListView.view.width : 390
    height: 64

    function tagText() {
        if (root.sourceText === qsTr("最近使用") || root.sourceText === qsTr("热门城市"))
            return root.sourceText
        if (root.levelText === "province")
            return qsTr("省级")
        if (root.levelText === "city")
            return qsTr("城市")
        if (root.levelText === "district")
            return qsTr("区县")
        if (root.levelText === "street")
            return qsTr("街道")
        return qsTr("行政区")
    }

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        radius: 13
        color: root.highlighted || mouseArea.containsMouse ? "#2D435C" : "transparent"
        border.width: root.highlighted ? 1 : 0
        border.color: "#547EAB"

        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Rectangle {
        width: 32
        height: 32
        radius: 16
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        color: root.sourceText === qsTr("最近使用") ? "#315A78" : "#263B50"

        Text {
            anchors.centerIn: parent
            text: root.sourceText === qsTr("最近使用") ? "↻" : "⌖"
            color: "#DFFFFFFF"
            font.pixelSize: 15
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 62
        anchors.top: parent.top
        anchors.topMargin: 10
        width: 205
        text: root.cityName
        color: "#FFFFFF"
        font.pixelSize: 15
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 62
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 9
        width: 232
        text: root.detailText.length > 0
              ? root.detailText
              : qsTr("点击按行政区中心坐标刷新天气")
        color: "#79FFFFFF"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        width: sourceLabel.implicitWidth + 18
        height: 25
        radius: 13
        color: "#1FFFFFFF"
        border.width: root.levelText.length > 0 ? 1 : 0
        border.color: "#294F769B"

        Text {
            id: sourceLabel
            anchors.centerIn: parent
            text: root.tagText()
            color: "#AFFFFFFF"
            font.pixelSize: 11
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.chosen()
    }
}
