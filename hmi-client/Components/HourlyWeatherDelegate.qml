import QtQuick

Rectangle {
    id: root

    property string timeText: "--:--"
    property string condition: "--"
    property int weatherCode: -1
    property int temperature: 0
    property int precipitation: 0
    property int humidity: 0
    property bool currentHour: false

    radius: 18
    color: currentHour ? "#314C72" : "#17202D"
    border.width: 1
    border.color: currentHour ? "#6FA8F6" : "#20FFFFFF"

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        text: root.timeText
        color: root.currentHour ? "#FFFFFF" : "#AFFFFFFF"
        font.pixelSize: 13
        font.weight: root.currentHour ? Font.DemiBold : Font.Normal
    }

    WeatherIcon {
        width: 48
        height: 48
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 31
        weatherCode: root.weatherCode
        sourcePixelSize: 112
        accessibleName: root.condition
    }

    Text {
        width: parent.width - 12
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 80
        text: root.condition
        color: "#D8FFFFFF"
        font.pixelSize: 11
        font.weight: root.currentHour ? Font.DemiBold : Font.Normal
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 101
        text: root.temperature + "°"
        color: "#FFFFFF"
        font.pixelSize: 24
        font.weight: Font.DemiBold
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        text: "降水 " + root.precipitation + "%"
        color: root.precipitation >= 60 ? "#75B9FF" : "#78FFFFFF"
        font.pixelSize: 11
    }
}
