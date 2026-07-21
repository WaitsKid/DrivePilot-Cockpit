import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string color: "#FFFFFF"
    property string backgroundColor: "#80000000"
    property string fontColor: "#FFFFFF"
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
    property int maxValue: 100
    property int value: 50
    property int autoMode: 0

    // controlled=true 时组件只发出用户编辑意图，不覆盖外部属性绑定。
    property bool controlled: false
    signal valueEdited(int value)

    Rectangle {
        id: backgroundRectangle
        anchors.fill: parent
        color: root.backgroundColor
        radius: 14

        Rectangle {
            id: innerRectangle
            width: slider.to > slider.from
                   ? Math.max(0, Math.min(parent.width,
                       (slider.value - slider.from) / (slider.to - slider.from) * parent.width))
                   : 0
            height: parent.height
            x: 0
            anchors.verticalCenter: parent.verticalCenter
            color: root.fontColor
            radius: 14
        }

        Slider {
            id: slider
            anchors.fill: parent
            z: image.z + 1
            from: root.minValue
            to: root.maxValue
            stepSize: 1
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

            onMoved: {
                const editedValue = Math.round(value)
                if (root.controlled)
                    root.valueEdited(editedValue)
                else
                    root.value = editedValue
            }
        }

        Binding {
            target: slider
            property: "value"
            value: root.value
            when: !slider.pressed
            restoreMode: Binding.RestoreBinding
        }

        Image {
            id: image
            width: sourceWidth
            height: sourceHeight
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.verticalCenter: parent.verticalCenter
            source: slider.value > 0 ? root.sourceOn : root.sourceOff
            fillMode: Image.PreserveAspectFit
        }

        Label {
            id: label1
            width: root.textWidth
            height: root.textHeight
            anchors.left: image.right
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: autoMode === 0 ? 33 : 24
            verticalAlignment: Text.AlignVCenter
            text: root.text
            color: textColor
            font.pixelSize: fontPixelSize
        }

        Label {
            width: root.textWidth
            height: root.textHeight
            anchors.left: image.right
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 52
            verticalAlignment: Text.AlignVCenter
            text: autoMode === 1 ? qsTr("自动") : qsTr("手动")
            color: autoTextColor
            font.pixelSize: autoFontPixelSize
            visible: autoMode !== 0
        }
    }
}
