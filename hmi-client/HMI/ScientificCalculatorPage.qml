import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0

    property var graphPoints: []
    property real graphMinimumX: -10
    property real graphMaximumX: 10

    function insertToken(token) {
        const start = expressionField.selectionStart
        const end = expressionField.selectionEnd
        const source = expressionField.text
        expressionField.text = source.slice(0, start) + token + source.slice(end)
        expressionField.cursorPosition = start + token.length
        expressionField.forceActiveFocus()
        graphDebounce.restart()
    }

    function removeToken() {
        if (expressionField.selectionStart !== expressionField.selectionEnd) {
            const start = expressionField.selectionStart
            const end = expressionField.selectionEnd
            expressionField.text = expressionField.text.slice(0, start)
                    + expressionField.text.slice(end)
            expressionField.cursorPosition = start
        } else if (expressionField.cursorPosition > 0) {
            const position = expressionField.cursorPosition
            expressionField.text = expressionField.text.slice(0, position - 1)
                    + expressionField.text.slice(position)
            expressionField.cursorPosition = position - 1
        }
        graphDebounce.restart()
    }

    function updateGraph() {
        const minimum = Number(minXField.text)
        const maximum = Number(maxXField.text)
        if (!isFinite(minimum) || !isFinite(maximum) || maximum <= minimum)
            return
        graphMinimumX = minimum
        graphMaximumX = maximum
        graphPoints = Calculator.sampleFunction(expressionField.text, minimum, maximum, 720)
        graphCanvas.requestPaint()
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#24030A15"
    }

    PageChrome { anchors.fill: parent }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 54
        anchors.top: parent.top
        anchors.topMargin: 62
        text: qsTr("高级科学计算器")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Button {
        id: angleModeButton
        width: 92
        height: 38
        anchors.left: parent.left
        anchors.leftMargin: 350
        anchors.top: parent.top
        anchors.topMargin: 61
        hoverEnabled: false
        contentItem: Label {
            text: Calculator.degreeMode ? qsTr("DEG 角度") : qsTr("RAD 弧度")
            color: "#FFFFFF"
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 14
            color: angleModeButton.down ? "#46648C" : "#2B3D58"
            border.width: 1
            border.color: "#5C7CFF"
        }
        onClicked: {
            Calculator.degreeMode = !Calculator.degreeMode
            root.updateGraph()
        }
    }

    Rectangle {
        id: calculatorPanel
        x: 48
        y: 116
        width: 754
        height: 580
        radius: 24
        color: "#C9151D28"
        border.width: 1
        border.color: "#24FFFFFF"

        TextField {
            id: expressionField
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 22
            verticalAlignment:Text.AlignVCenter
            height: 58
            text: "sin(x) / x"
            placeholderText: qsTr("输入表达式，例如 sin(x) / x")
            color: "#FFFFFF"
            font.pixelSize: 22
            leftPadding: 18
            rightPadding: 18
            selectByMouse: true
            background: Rectangle {
                radius: 17
                color: "#243247"
                border.width: expressionField.activeFocus ? 2 : 1
                border.color: expressionField.activeFocus ? "#70A7F5" : "#2FFFFFFF"
            }
            onTextEdited: graphDebounce.restart()
            Keys.onReturnPressed: Calculator.calculate(text)
        }

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: expressionField.bottom
            anchors.topMargin: 12
            height: 72
            radius: 17
            color: "#151F2E"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                text: Calculator.errorString.length > 0
                      ? Calculator.errorString : Calculator.resultText
                color: Calculator.errorString.length > 0 ? "#FF8A8A" : "#72E6C5"
                font.pixelSize: Calculator.errorString.length > 0 ? 16 : 28
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                width: parent.width - 36
            }
        }

        Row {
            id: advancedActions
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: expressionField.bottom
            anchors.topMargin: 96
            height: 44
            spacing: 10

            TextField {
                id: lowerField
                width: 82
                height: 42
                text: "0"
                verticalAlignment:Text.AlignVCenter
                placeholderText: qsTr("下限")
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                validator: DoubleValidator {}
                background: Rectangle { radius: 13; color: "#243247"; border.color: "#2FFFFFFF" }
            }
            TextField {
                id: upperField
                width: 82
                height: 42
                text: "1"
                verticalAlignment:Text.AlignVCenter
                placeholderText: qsTr("上限")
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                validator: DoubleValidator {}
                background: Rectangle { radius: 13; color: "#243247"; border.color: "#2FFFFFFF" }
            }
            Button {
                width: 106
                height: 42
                text: qsTr("数值积分")
                onClicked: Calculator.calculateIntegral(
                               expressionField.text,
                               Number(lowerField.text), Number(upperField.text))
            }
            TextField {
                id: limitField
                width: 82
                height: 42
                text: "0"
                verticalAlignment:Text.AlignVCenter
                placeholderText: qsTr("趋近点")
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                validator: DoubleValidator {}
                background: Rectangle { radius: 13; color: "#243247"; border.color: "#2FFFFFFF" }
            }
            Button {
                width: 106
                height: 42
                text: qsTr("求极限")
                onClicked: Calculator.calculateLimit(
                               expressionField.text, Number(limitField.text))
            }
            Button {
                width: 106
                height: 42
                text: qsTr("直接计算")
                onClicked: Calculator.calculate(expressionField.text)
            }
        }

        GridLayout {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: advancedActions.bottom
            anchors.topMargin: 15
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 22
            columns: 6
            rowSpacing: 9
            columnSpacing: 9

            Repeater {
                model: [
                    {t:"7", v:"7"}, {t:"8", v:"8"}, {t:"9", v:"9"}, {t:"÷", v:"/", a:true}, {t:"sin", v:"sin("}, {t:"(", v:"("},
                    {t:"4", v:"4"}, {t:"5", v:"5"}, {t:"6", v:"6"}, {t:"×", v:"*", a:true}, {t:"cos", v:"cos("}, {t:")", v:")"},
                    {t:"1", v:"1"}, {t:"2", v:"2"}, {t:"3", v:"3"}, {t:"−", v:"-", a:true}, {t:"tan", v:"tan("}, {t:"^", v:"^"},
                    {t:"0", v:"0"}, {t:".", v:"."}, {t:"x", v:"x"}, {t:"+", v:"+", a:true}, {t:"√", v:"sqrt("}, {t:"⌫", action:"back"},
                    {t:"π", v:"pi"}, {t:"e", v:"e"}, {t:"ln", v:"ln("}, {t:"log", v:"log("}, {t:"abs", v:"abs("}, {t:"AC", action:"clear", a:true}
                ]

                CalculatorKey {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: modelData.t
                    accent: modelData.a === true
                    onClicked: {
                        if (modelData.action === "back")
                            root.removeToken()
                        else if (modelData.action === "clear") {
                            expressionField.clear()
                            expressionField.forceActiveFocus()
                            graphDebounce.restart()
                        } else {
                            root.insertToken(modelData.v)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: graphPanel
        x: 826
        y: 116
        width: 536
        height: 360
        radius: 24
        color: "#C9151D28"
        border.width: 1
        border.color: "#24FFFFFF"

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 16
            text: qsTr("实时函数图像")
            color: "#FFFFFF"
            font.pixelSize: 19
            font.weight: Font.DemiBold
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 8

            TextField {
                id: minXField
                width: 66
                height: 34
                text: "-10"
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                validator: DoubleValidator {}
                background: Rectangle { radius: 11; color: "#243247"; border.color: "#2FFFFFFF" }
            }
            Label { text: "～"; color: "#8FFFFFFF"; anchors.verticalCenter: parent.verticalCenter }
            TextField {
                id: maxXField
                width: 66
                height: 34
                text: "10"
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                validator: DoubleValidator {}
                background: Rectangle { radius: 11; color: "#243247"; border.color: "#2FFFFFFF" }
            }
            Button {
                width: 70
                height: 34
                text: qsTr("重绘")
                onClicked: root.updateGraph()
            }
        }

        Canvas {
            id: graphCanvas
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 58
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16

            onPaint: {
                const context = getContext("2d")
                context.clearRect(0, 0, width, height)
                context.fillStyle = "#111A28"
                context.fillRect(0, 0, width, height)
                if (!root.graphPoints || root.graphPoints.length < 2)
                    return

                var minimumY = Infinity
                var maximumY = -Infinity
                for (var i = 0; i < root.graphPoints.length; ++i) {
                    const y = root.graphPoints[i].y
                    if (isFinite(y) && Math.abs(y) < 1000000) {
                        minimumY = Math.min(minimumY, y)
                        maximumY = Math.max(maximumY, y)
                    }
                }
                if (!isFinite(minimumY) || !isFinite(maximumY))
                    return
                if (Math.abs(maximumY - minimumY) < 0.000001) {
                    minimumY -= 1
                    maximumY += 1
                }
                const padding = (maximumY - minimumY) * 0.12
                minimumY -= padding
                maximumY += padding

                function mapX(x) {
                    return (x - root.graphMinimumX)
                            / (root.graphMaximumX - root.graphMinimumX) * width
                }
                function mapY(y) {
                    return height - (y - minimumY) / (maximumY - minimumY) * height
                }

                context.strokeStyle = "#26364D"
                context.lineWidth = 1
                for (var grid = 1; grid < 10; ++grid) {
                    context.beginPath()
                    context.moveTo(width * grid / 10, 0)
                    context.lineTo(width * grid / 10, height)
                    context.stroke()
                    context.beginPath()
                    context.moveTo(0, height * grid / 10)
                    context.lineTo(width, height * grid / 10)
                    context.stroke()
                }

                context.strokeStyle = "#6F86A8"
                context.lineWidth = 1.4
                if (root.graphMinimumX <= 0 && root.graphMaximumX >= 0) {
                    const zeroX = mapX(0)
                    context.beginPath(); context.moveTo(zeroX, 0); context.lineTo(zeroX, height); context.stroke()
                }
                if (minimumY <= 0 && maximumY >= 0) {
                    const zeroY = mapY(0)
                    context.beginPath(); context.moveTo(0, zeroY); context.lineTo(width, zeroY); context.stroke()
                }

                context.strokeStyle = "#52E6FB"
                context.lineWidth = 2.4
                context.beginPath()
                var drawing = false
                for (var pointIndex = 0; pointIndex < root.graphPoints.length; ++pointIndex) {
                    const point = root.graphPoints[pointIndex]
                    if (!isFinite(point.y) || Math.abs(point.y) > 1000000) {
                        drawing = false
                        continue
                    }
                    const px = mapX(point.x)
                    const py = mapY(point.y)
                    if (!drawing) {
                        context.moveTo(px, py)
                        drawing = true
                    } else {
                        context.lineTo(px, py)
                    }
                }
                context.stroke()
            }
        }
    }

    Rectangle {
        x: 826
        y: 492
        width: 536
        height: 204
        radius: 24
        color: "#C9151D28"
        border.width: 1
        border.color: "#24FFFFFF"

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 15
            text: qsTr("计算历史")
            color: "#FFFFFF"
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }

        Button {
            width: 70
            height: 32
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 10
            text: qsTr("清空")
            onClicked: Calculator.clearHistory()
        }

        ListView {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 52
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            model: Calculator.history
            clip: true
            spacing: 7

            delegate: Rectangle {
                required property var modelData
                width: ListView.view.width
                height: 48
                radius: 13
                color: "#202C3E"

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: resultLabel.left
                    anchors.rightMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 6
                    text: modelData.operation + " · " + modelData.expression
                    color: "#CFFFFFFF"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    text: modelData.time
                    color: "#65FFFFFF"
                    font.pixelSize: 10
                }
                Label {
                    id: resultLabel
                    width: 150
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: "= " + modelData.result
                    color: "#72E6C5"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideLeft
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }

    Timer {
        id: graphDebounce
        interval: 260
        repeat: false
        onTriggered: root.updateGraph()
    }

    Component.onCompleted: root.updateGraph()
}
