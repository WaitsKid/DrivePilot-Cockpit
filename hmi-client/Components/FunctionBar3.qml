import QtQuick
import QtQuick.Controls

// 空调控制栏
Item {
    id: root

    property string background: "#5A364A5E"
    property string textColor: "#FFFFFF"
    property string function1Text: ""
    property string function2Text: ""
    property string function3Text: ""
    property int fontPixelSize: 16
    property int buttonWidth: width / 3

    property int functionValue: 0

    // 背景
    Rectangle {
        anchors.fill: parent
        color: parent.background
        radius: 43
    }

    // 按钮背景
    Rectangle {
        id: selectedRectangle
        width: buttonWidth
        height: parent.height
        y: 0
        radius: 51
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#43FFFF" }
            GradientStop { position: 1.0; color: "#0978E9" }
        }

        state: getState()

        function getState()
        {
            switch(functionValue)
            {
                case 0: return "FUNCTION1"
                case 1: return "FUNCTION2"
                case 2: return "FUNCTION3"
            }
        }

        states: [
            State {
                name: "FUNCTION1"
                PropertyChanges {
                    target: selectedRectangle
                    x: 0
                }
            },
            State {
                name: "FUNCTION2"
                PropertyChanges {
                    target: selectedRectangle
                    x: buttonWidth
                }
            },
            State {
                name: "FUNCTION3"
                PropertyChanges {
                    target: selectedRectangle
                    x: buttonWidth * 2
                }
            }
        ]

        transitions: [
            Transition {
                to: "FUNCTION1"
                PropertyAnimation {
                    target: selectedRectangle
                    properties: "x"
                    easing.type: Easing.InOutQuad
                    duration: 300
                }
            },
            Transition {
                to: "FUNCTION2"
                PropertyAnimation {
                    target: selectedRectangle
                    properties: "x"
                    easing.type: Easing.InOutQuad
                    duration: 300
                }
            },
            Transition {
                to: "FUNCTION3"
                PropertyAnimation {
                    target: selectedRectangle
                    properties: "x"
                    easing.type: Easing.InOutQuad
                    duration: 300
                }
            }
        ]
    }


    // 按钮
    Row {
        anchors.fill: parent

        RadioButton {
            id: button1
            width: buttonWidth
            height: parent.height
            checked: true

            indicator: Rectangle {color: "transparent"}

            Label {
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: function1Text
                color: textColor
                font.pixelSize: fontPixelSize
            }

            onCheckedChanged: checked ? functionValue = 0 : ""
        }

        RadioButton {
            id: button2
            width: buttonWidth
            height: parent.height
            indicator: Rectangle {color: "transparent"}

            Label {
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: function2Text
                color: textColor
                font.pixelSize: fontPixelSize
            }

            onCheckedChanged: checked ? functionValue = 1 : ""
        }

        RadioButton {
            id: button3
            width: buttonWidth
            height: parent.height
            indicator: Rectangle {color: "transparent"}

            Label {
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: function3Text
                color: textColor
                font.pixelSize: fontPixelSize
            }

            onCheckedChanged: checked ? functionValue = 2 : ""
        }
    }

}
