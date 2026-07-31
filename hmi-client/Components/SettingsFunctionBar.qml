import QtQuick
import QtQuick.Controls
import DrivePilot

Item {
    id: root

    property int functionValue: 0
    readonly property var titles: [qsTr("智能底盘"), qsTr("灯光氛围"), qsTr("抬头显示"), qsTr("迎宾"), qsTr("智能记忆"), qsTr("空调"), qsTr("门窗和锁"), qsTr("智能提醒")]
    readonly property real buttonWidth: width / titles.length
    readonly property real indicatorWidth: 52
    readonly property real indicatorOffset: (buttonWidth - indicatorWidth) / 2

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Settings/function_background.png"
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: indicatorRectangle
        width: indicatorWidth
        height: 9
        y: parent.height - height
        radius: height / 2
        color: "#59EBFD"
        x: root.functionValue * root.buttonWidth + root.indicatorOffset

        Behavior on x {
            NumberAnimation {
                duration: 220
                easing.type: Easing.InOutQuad
            }
        }
    }

    Row {
        anchors.fill: parent

        Repeater {
            model: root.titles.length

            delegate: Item {
                width: root.buttonWidth
                height: parent.height

                Label {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: root.titles[index]
                    color: index === root.functionValue ? "#FFFFFF" : "#CCFFFFFF"
                    font.pixelSize: 20
                    font.bold: index === root.functionValue
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: Ui.settingsFunctionValue = index
                }
            }
        }
    }
}
