import QtQuick
import QtQuick.Controls
import BYD

Item {
    width: 1414
    height: 856
    x: 108
    y: 0

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    PropertyAnimation {
        id: fadeInAnimation
        target: parent
        properties: "opacity"
        duration: 500
        from: 0
        to: 1
        easing.type: Easing.OutQuad
    }

    Component.onCompleted: fadeInAnimation.start()

    SettingsModeBar {
        id: leftBackgroundImage
        width: 240
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
        functionValue: 3
    }

    SettingsFunctionBar {
        id: settingsFunctionBar
        width: 1099
        height: 69
        anchors.left: parent.left
        anchors.leftMargin: 276
        anchors.top: parent.top
        anchors.topMargin: 81
        functionValue: Ui.settingsFunctionValue
    }

    Image {
        id: centerBackgroundImage
        width: 1099
        height: 705
        anchors.left: parent.left
        anchors.leftMargin: 276
        anchors.top: parent.top
        anchors.topMargin: 151
        source: "qrc:/Images/Settings/center_background.png"
        fillMode: Image.PreserveAspectFit
    }

    SettingsList {
        id: settingsList
        width: 537
        height: 705
        contentWidth: 405
        contentHeight: 3520
        anchors.left: parent.left
        anchors.leftMargin: 276
        anchors.top: parent.top
        anchors.topMargin: 151
    }

    Rectangle {
        width: 428
        height: 110
        radius: 24
        color: "#301B2838"
        border.width: 1
        border.color: "#22FFFFFF"
        anchors.left: parent.left
        anchors.leftMargin: 827
        anchors.top: parent.top
        anchors.topMargin: 171

        Column {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            Label {
                text: qsTr("车辆设置")
                color: "#FFFFFF"
                font.pixelSize: 28
                font.bold: true
            }

            Row {
                spacing: 18

                Label {
                    text: qsTr("安全陪伴 267 天")
                    color: "#9AFFFFFF"
                    font.pixelSize: 16
                }
                Label {
                    text: qsTr("建议电量维护：80%")
                    color: "#9AFFFFFF"
                    font.pixelSize: 16
                }
            }
        }
    }

    Image {
        width: 472
        height: 182
        anchors.left: parent.left
        anchors.leftMargin: 870
        anchors.top: parent.top
        anchors.topMargin: 410
        source: "qrc:/Images/Settings/vehicle.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: vehicleConditionImage
        width: 167
        height: 28
        anchors.left: parent.left
        anchors.leftMargin: 1160
        anchors.top: parent.top
        anchors.topMargin: 637
        source: "qrc:/Images/Settings/vehicle_condition_good.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: milesImage
        width: 149
        height: 74
        anchors.left: parent.left
        anchors.leftMargin: 1178
        anchors.top: parent.top
        anchors.topMargin: 294
        source: "qrc:/Images/Settings/miles.png"
        fillMode: Image.PreserveAspectFit
    }

    PageChrome {
        anchors.fill: parent
    }
}
