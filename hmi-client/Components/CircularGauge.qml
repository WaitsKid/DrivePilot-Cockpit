import QtQuick
import QtQuick.Controls

Item {
    id: root

    property real value: 0
    property real minimumValue: 0
    property real maximumValue: 100
    property string title: ""
    property string unit: ""
    property color progressColor: "#48E5C2"
    property color trackColor: "#2DFFFFFF"
    property color tickColor: "#55FFFFFF"
    property color valueColor: "#FFFFFF"

    readonly property real normalizedValue: Math.max(0,
                                                     Math.min(1,
                                                              (value - minimumValue)
                                                              / Math.max(1, maximumValue - minimumValue)))

    onValueChanged: gaugeCanvas.requestPaint()
    onMinimumValueChanged: gaugeCanvas.requestPaint()
    onMaximumValueChanged: gaugeCanvas.requestPaint()
    onProgressColorChanged: gaugeCanvas.requestPaint()
    onTrackColorChanged: gaugeCanvas.requestPaint()
    onTickColorChanged: gaugeCanvas.requestPaint()

    Behavior on value {
        NumberAnimation {
            duration: 420
            easing.type: Easing.OutCubic
        }
    }

    Canvas {
        id: gaugeCanvas
        anchors.fill: parent
        antialiasing: true

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var context = getContext("2d")
            context.clearRect(0, 0, width, height)

            var centerX = width / 2
            var centerY = height / 2
            var radius = Math.min(width, height) * 0.40
            var lineWidth = Math.max(12, Math.min(width, height) * 0.065)
            var startAngle = Math.PI * 0.75
            var sweepAngle = Math.PI * 1.5

            context.lineCap = "round"
            context.lineWidth = lineWidth

            context.beginPath()
            context.strokeStyle = root.trackColor
            context.arc(centerX, centerY, radius, startAngle, startAngle + sweepAngle, false)
            context.stroke()

            context.beginPath()
            context.strokeStyle = root.progressColor
            context.arc(centerX,
                        centerY,
                        radius,
                        startAngle,
                        startAngle + sweepAngle * root.normalizedValue,
                        false)
            context.stroke()

            context.lineCap = "butt"
            context.lineWidth = 2
            context.strokeStyle = root.tickColor
            for (var index = 0; index <= 10; ++index) {
                var angle = startAngle + sweepAngle * index / 10
                var outerRadius = radius + lineWidth * 0.88
                var innerRadius = outerRadius - (index % 5 === 0 ? 12 : 7)

                context.beginPath()
                context.moveTo(centerX + Math.cos(angle) * innerRadius,
                               centerY + Math.sin(angle) * innerRadius)
                context.lineTo(centerX + Math.cos(angle) * outerRadius,
                               centerY + Math.sin(angle) * outerRadius)
                context.stroke()
            }
        }
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 5
        spacing: 1

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.round(root.value)
            color: root.valueColor
            font.pixelSize: Math.max(42, root.width * 0.23)
            font.bold: true
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.unit
            color: "#AFFFFFFF"
            font.pixelSize: Math.max(14, root.width * 0.075)
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.title
            color: "#82FFFFFF"
            font.pixelSize: Math.max(13, root.width * 0.065)
        }
    }
}
