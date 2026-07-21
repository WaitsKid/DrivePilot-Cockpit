import QtQuick
import QtQuick.Controls

Item {
    id: root

    property int flowState: 0
    property int idleState: 0
    property int driveState: 1
    property int regenState: 2

    readonly property bool active: flowState !== idleState
    readonly property bool reverseFlow: flowState === regenState
    readonly property color flowColor: reverseFlow ? "#44E4A6" : "#4D8DFF"

    Rectangle {
        id: shadow
        width: 260
        height: 374
        radius: 105
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 7
        color: "#26000000"
    }

    Rectangle {
        id: carBody
        width: 224
        height: 350
        radius: 88
        anchors.centerIn: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#40516A" }
            GradientStop { position: 0.45; color: "#263244" }
            GradientStop { position: 1.0; color: "#151D29" }
        }
        border.width: 2
        border.color: "#58708D"

        Rectangle {
            width: 154
            height: 92
            radius: 34
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 51
            color: "#162638"
            border.width: 1
            border.color: "#39506A"

            Rectangle {
                width: 2
                height: parent.height
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#284157"
            }
        }

        Rectangle {
            id: motor
            width: 72
            height: 72
            radius: 36
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 116
            color: "#253C57"
            border.width: 3
            border.color: root.flowColor

            Label {
                anchors.centerIn: parent
                text: qsTr("电机")
                color: "#FFFFFF"
                font.pixelSize: 14
                font.bold: true
            }
        }

        Rectangle {
            id: battery
            width: 142
            height: 92
            radius: 18
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 45
            color: "#263F3B"
            border.width: 2
            border.color: "#43C4A3"

            Row {
                anchors.centerIn: parent
                spacing: 6

                Repeater {
                    model: 4
                    Rectangle {
                        width: 23
                        height: 48
                        radius: 5
                        color: "#2A9E82"
                        opacity: 0.65 + index * 0.08
                    }
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5
                text: qsTr("动力电池")
                color: "#DFFFFFFF"
                font.pixelSize: 11
            }
        }

        Rectangle {
            id: centerFlowLine
            width: 6
            height: battery.y - (motor.y + motor.height) + 4
            radius: 3
            anchors.horizontalCenter: parent.horizontalCenter
            y: motor.y + motor.height - 2
            color: root.flowColor
            opacity: root.active ? 0.55 : 0.16
        }

        Repeater {
            model: 4
            Rectangle {
                width: 11
                height: 11
                radius: 6
                x: carBody.width / 2 - width / 2
                color: root.flowColor
                opacity: root.active ? 1 : 0
                layer.enabled: true

                SequentialAnimation on y {
                    running: root.active
                    loops: Animation.Infinite
                    PauseAnimation { duration: index * 170 }
                    NumberAnimation {
                        from: root.reverseFlow ? battery.y + 15 : motor.y + motor.height
                        to: root.reverseFlow ? motor.y + motor.height : battery.y + 15
                        duration: 850
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }

        Rectangle {
            width: 178
            height: 6
            radius: 3
            anchors.horizontalCenter: parent.horizontalCenter
            y: motor.y + motor.height / 2 - height / 2
            color: root.flowColor
            opacity: root.active ? 0.48 : 0.14
        }

        Repeater {
            model: 3
            Rectangle {
                width: 10
                height: 10
                radius: 5
                y: motor.y + motor.height / 2 - height / 2
                color: root.flowColor
                opacity: root.active ? 1 : 0

                SequentialAnimation on x {
                    running: root.active
                    loops: Animation.Infinite
                    PauseAnimation { duration: index * 190 }
                    NumberAnimation {
                        from: root.reverseFlow ? 18 : carBody.width / 2 - 4
                        to: root.reverseFlow ? carBody.width / 2 - 4 : 18
                        duration: 700
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }

        Repeater {
            model: 3
            Rectangle {
                width: 10
                height: 10
                radius: 5
                y: motor.y + motor.height / 2 - height / 2
                color: root.flowColor
                opacity: root.active ? 1 : 0

                SequentialAnimation on x {
                    running: root.active
                    loops: Animation.Infinite
                    PauseAnimation { duration: index * 190 }
                    NumberAnimation {
                        from: root.reverseFlow ? carBody.width - 28 : carBody.width / 2 - 4
                        to: root.reverseFlow ? carBody.width / 2 - 4 : carBody.width - 28
                        duration: 700
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }
    }

    Repeater {
        model: 4
        Rectangle {
            width: 27
            height: 78
            radius: 11
            color: "#111820"
            border.width: 2
            border.color: "#4A5C70"
            x: index % 2 === 0 ? carBody.x - 18 : carBody.x + carBody.width - 9
            y: index < 2 ? carBody.y + 65 : carBody.y + carBody.height - 143

            Rectangle {
                width: 9
                height: 54
                radius: 5
                anchors.centerIn: parent
                color: "#293746"
            }
        }
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: root.flowState === root.regenState
              ? qsTr("制动能量回收")
              : (root.flowState === root.driveState ? qsTr("电池驱动车轮") : qsTr("车辆已驻车"))
        color: root.active ? root.flowColor : "#80FFFFFF"
        font.pixelSize: 14
        font.bold: true
    }
}
