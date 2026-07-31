import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string color: "#FFFFFF"
    property string backgroundColor: "#80000000"
    property string startColor: "#0532FB"
    property string endColor: "#52E6FB"

    property string sourceOn: ""
    property string sourceOff: ""
    property int sourceWidth: 36
    property int sourceHeight: 30
    property int spacing: 5

    property int textWidth: 10
    property int textHeight: 10
    property string text: ""
    property int fontPixelSize: 20
    property int autoFontPixelSize: 14
    property color textColor: "#FFFFFF"
    property color autoTextColor: "#80FFFFFF"

    property bool switchStatus: false

    property int minValue: 0
    property int maxValue: 10
    property int value: 5
    property int stepSize: 1
    property real cornerRadius: 14

    signal valueModified(int value)


    Rectangle {
        id: backroundRectangle
        anchors.fill: parent
        color: root.backgroundColor
        radius: root.cornerRadius

        Rectangle {
            id: innerRectangle
            width: getWidth()
            height: parent.height
            x: 0
            anchors.verticalCenter: parent.verticalCenter
            radius: root.cornerRadius

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: startColor }
                GradientStop { position: 1.0; color: endColor }
            }

            property real stepSize: backroundRectangle.width / (slider.to)

            function getWidth()
            {
                if(slider.value == slider.to)
                {
                    var width = (slider.value + 1) * stepSize
                    if(width >= backroundRectangle.width)
                    {
                        width = backroundRectangle.width
                    }

                    return width
                }
                else
                {
                    return slider.value * stepSize
                }
            }

            function getRadius()
            {
                switch(slider.value)
                {
                    case 1: return 2
                    case 2: return 4
                    case 3: return 6
                    case 4: return 8
                    case 5: return 10
                    case 6: return 12
                    default: return root.cornerRadius
                }
            }
        }

        Slider {
            id: slider
            anchors.fill: parent
            from: minValue
            to: maxValue
            stepSize: root.stepSize
            focusPolicy: Qt.NoFocus

            background: Rectangle {
                implicitWidth: 0
                implicitHeight: parent.height
                color: "transparent"
            }

            handle: Rectangle {
                implicitWidth: 0
                implicitHeight: parent.height
                color: "transparent"
            }
        }

        Binding {
            target: slider
            property: "value"
            value: root.value
            restoreMode: Binding.RestoreBinding
        }

        Connections {
            target: slider
            function onValueChanged() {
                if (slider.value !== root.value)
                    root.valueModified(Math.round(slider.value))
            }
        }
    }
}
