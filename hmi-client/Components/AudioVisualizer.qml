import QtQuick

Item {
    id: root

    property bool active: false
    property color barColor: "#70FFFFFF"
    property int barCount: 14

    Row {
        anchors.fill: parent
        spacing: Math.max(2, (width - root.barCount * 4) / Math.max(1, root.barCount - 1))

        Repeater {
            model: root.barCount

            Item {
                required property int index
                width: 4
                height: parent.height

                Rectangle {
                    id: bar
                    width: parent.width
                    height: 4
                    radius: width / 2
                    anchors.bottom: parent.bottom
                    color: root.barColor

                    SequentialAnimation {
                        id: barAnimation
                        running: root.active
                        loops: Animation.Infinite

                        NumberAnimation {
                            target: bar
                            property: "height"
                            to: 8 + ((index * 17 + 9) % Math.max(12, root.height - 5))
                            duration: 190 + index * 13
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            target: bar
                            property: "height"
                            to: 5 + ((index * 7 + 3) % Math.max(10, root.height - 8))
                            duration: 230 + index * 9
                            easing.type: Easing.InOutSine
                        }

                        onRunningChanged: {
                            if (!running)
                                bar.height = 4
                        }
                    }
                }
            }
        }
    }
}
