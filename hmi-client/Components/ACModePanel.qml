import QtQuick
import QtQuick.Controls
import BYD

Item {
    id: root

    anchors.fill: parent
    visible: false
    opacity: 0
    z: 1200

    readonly property bool opened: visible

    function open() {
        closeTimer.stop()
        visible = true
        opacity = 1
    }

    function close() {
        opacity = 0
        closeTimer.restart()
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutCubic
        }
    }

    Timer {
        id: closeTimer
        interval: 190
        repeat: false
        onTriggered: root.visible = false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        id: panel
        width: 760
        height: 330
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -18
        radius: 30
        color: "#F2222B3A"
        border.width: 1
        border.color: "#406FE9FF"

        MouseArea {
            anchors.fill: parent
        }

        Label {
            id: titleLabel
            anchors.left: parent.left
            anchors.leftMargin: 34
            anchors.top: parent.top
            anchors.topMargin: 28
            text: qsTr("空调运行模式")
            color: "#FFFFFF"
            font.pixelSize: 27
            font.weight: Font.DemiBold
        }

        Label {
            anchors.left: titleLabel.left
            anchors.top: titleLabel.bottom
            anchors.topMargin: 8
            text: qsTr("当前：%1模式 · 风量 %2 档").arg(Ui.acModeText).arg(Ui.acFanLevel)
            color: "#AFFFFFFF"
            font.pixelSize: 16
        }

        Button {
            width: 44
            height: 44
            anchors.right: parent.right
            anchors.rightMargin: 22
            anchors.top: parent.top
            anchors.topMargin: 20
            hoverEnabled: false

            contentItem: Label {
                text: "×"
                color: "#DFFFFFFF"
                font.pixelSize: 30
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 22
                color: parent.down ? "#35FFFFFF" : "#18FFFFFF"
            }
            onClicked: root.close()
        }

        Grid {
            anchors.left: parent.left
            anchors.leftMargin: 34
            anchors.right: parent.right
            anchors.rightMargin: 34
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 30
            columns: 4
            columnSpacing: 14

            Repeater {
                model: [
                    { value: Ui.AC_MODE_NORMAL, title: qsTr("正常"), detail: qsTr("均衡舒适\n风量自动设为 5 档") },
                    { value: Ui.AC_MODE_DRY, title: qsTr("除湿"), detail: qsTr("降低湿度\n保持当前风量") },
                    { value: Ui.AC_MODE_BOOST, title: qsTr("加强"), detail: qsTr("快速制冷/制热\n风量自动设为 10 档") },
                    { value: Ui.AC_MODE_AUTO, title: qsTr("自动"), detail: qsTr("智能调节\n保持当前风量") }
                ]

                delegate: Button {
                    required property var modelData
                    width: 162
                    height: 176
                    hoverEnabled: false

                    contentItem: Item {
                        Rectangle {
                            width: 54
                            height: 54
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 20
                            radius: 27
                            color: Ui.acMode === modelData.value ? "#3979E8" : "#243143"
                            border.width: 1
                            border.color: Ui.acMode === modelData.value ? "#7FC8FF" : "#25FFFFFF"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.value === Ui.AC_MODE_NORMAL ? "N"
                                      : modelData.value === Ui.AC_MODE_DRY ? "D"
                                      : modelData.value === Ui.AC_MODE_BOOST ? "B"
                                      : "A"
                                color: "#FFFFFF"
                                font.pixelSize: 22
                                font.weight: Font.Bold
                            }
                        }

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 82
                            text: modelData.title
                            color: "#FFFFFF"
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        Label {
                            width: parent.width - 14
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 14
                            text: modelData.detail
                            color: "#AFFFFFFF"
                            font.pixelSize: 13
                            lineHeight: 1.25
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    background: Rectangle {
                        radius: 22
                        color: Ui.acMode === modelData.value
                               ? (parent.down ? "#3E5577" : "#324763")
                               : (parent.down ? "#2F3948" : "#242D3B")
                        border.width: Ui.acMode === modelData.value ? 2 : 1
                        border.color: Ui.acMode === modelData.value ? "#6FE9FF" : "#20FFFFFF"
                    }

                    onClicked: {
                        Ui.selectAcMode(modelData.value)
                        root.close()
                    }
                }
            }
        }
    }
}
