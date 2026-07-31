import QtQuick
import QtQuick.Controls
import DrivePilot

Item {
    id: root

    property int functionValue: 3
    property int buttonWidth: 204
    property int buttonHeight: 70
    readonly property var menuTitles: [qsTr("DiLink"), qsTr("DiPilot"), qsTr("新能源"), qsTr("车辆设置"), qsTr("车辆健康")]

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Settings/left_background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        id: selectedRectangle
        width: buttonWidth
        height: buttonHeight
        x: 18
        y: 449
        radius: 35
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#43FFFF" }
            GradientStop { position: 1.0; color: "#0978E9" }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.top: parent.top
        anchors.topMargin: 83
        spacing: 52

        Repeater {
            model: root.menuTitles.length

            delegate: Item {
                width: root.buttonWidth
                height: root.buttonHeight

                Label {
                    anchors.centerIn: parent
                    text: root.menuTitles[index]
                    color: index === 3 ? "#FFFFFF" : "#E5FFFFFF"
                    opacity: index === 3 ? 1 : 0.82
                    font.pixelSize: 24
                    font.bold: index === 3
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.functionValue = 3
                        if (index !== 3)
                            Ui.showToast(qsTr("请选择车辆设置"))
                    }
                }
            }
        }
    }
}
