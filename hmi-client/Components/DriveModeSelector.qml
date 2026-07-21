import QtQuick
import QtQuick.Controls

Item {
    id: root

    property int currentMode: 1
    signal modeSelected(int mode)

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: "#2518202C"
        border.width: 1
        border.color: "#28FFFFFF"
    }

    Row {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Repeater {
            model: 3

            Button {
                width: (root.width - 20) / 3
                height: root.height - 10
                hoverEnabled: false

                property string modeText: index === 0 ? qsTr("经济")
                                                       : (index === 1 ? qsTr("标准")
                                                                      : qsTr("运动"))

                contentItem: Label {
                    text: parent.modeText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: root.currentMode === index ? "#FFFFFF" : "#85FFFFFF"
                    font.pixelSize: 15
                    font.bold: root.currentMode === index
                }

                background: Rectangle {
                    radius: height / 2
                    color: root.currentMode === index
                           ? (index === 0 ? "#2DBD9E"
                                          : (index === 1 ? "#3776E8" : "#EC6A5E"))
                           : "transparent"

                    Behavior on color {
                        ColorAnimation { duration: 180 }
                    }
                }

                onClicked: root.modeSelected(index)
            }
        }
    }
}
