import QtQuick

Rectangle {
    id: root

    property string weekdayText: "--"
    property string dateText: "--"
    property string condition: "--"
    property int weatherCode: -1
    property int highTemperature: 0
    property int lowTemperature: 0
    property int precipitation: 0
    property bool today: false

    radius: 20
    color: today ? "#253E5F" : "#161F2C"
    border.width: 1
    border.color: today ? "#5E93D8" : "#20FFFFFF"

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 7
        text: root.weekdayText
        color: "#FFFFFF"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 26
        text: root.dateText
        color: "#6FFFFFFF"
        font.pixelSize: 10
    }

    WeatherIcon {
        width: 38
        height: 38
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 38
        weatherCode: root.weatherCode
        sourcePixelSize: 96
        accessibleName: root.condition
    }

    Text {
        width: parent.width - 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 74
        text: root.condition
        color: "#C8FFFFFF"
        font.pixelSize: 10
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 89
        text: root.highTemperature + "°  /  " + root.lowTemperature + "°"
        color: "#FFFFFF"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        text: "降水 " + root.precipitation + "%"
        color: root.precipitation >= 60 ? "#75B9FF" : "#72FFFFFF"
        font.pixelSize: 10
    }
}
