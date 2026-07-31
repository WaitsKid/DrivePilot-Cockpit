import QtQuick
import QtQuick.Controls
Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0
    property int leftPercentX: 0
    property int leftPercentY: 100
    property int rightPercentX: 0
    property int rightPercentY: 100
    property int xStepSize: rightRect.width / 100
    property int yStepSize: rightRect.height / 100
    property var colorArray: ["#5055AAFF", "#5077BBF8", "#5088BBF0",
        "#5099CCEE", "#5099DDEE", "#50AAEEEE",
        "#50BBEEEE", "#50CCDDEE", "#50DDDDFF",
        "#50EEEEFF", "#50FFFFFF", "#50FFEEEE",
        "#50FFDDDD", "#50FFCCCC", "#50EEBBBB",
        "#50EEAAAA", "#50EE9999"]

    function getWindColor(temperature)
    {
        switch(temperature)
        {
            case 16: return colorArray[0]
            case 17: return colorArray[1]
            case 18: return colorArray[2]
            case 19: return colorArray[3]
            case 20: return colorArray[4]
            case 21: return colorArray[5]
            case 22: return colorArray[6]
            case 23: return colorArray[7]
            case 24: return colorArray[8]
            case 25: return colorArray[9]
            case 26: return colorArray[10]
            case 27: return colorArray[11]
            case 28: return colorArray[12]
            case 29: return colorArray[13]
            case 30: return colorArray[14]
            case 31: return colorArray[15]
            case 32: return colorArray[16]
        }
    }
    Image {
        id: backgroundImage
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
        //内饰
        Image{
            id:innerImage
            width: 1414
            height: 707
            x: 0
            y: 0
            source: "qrc:/Images/AC/inner.png"
            fillMode: Image.Stretch
            // 垂直遮罩
            Image {
                id: maskVImage
                anchors.fill: parent
                source: "qrc:/Images/AC/mask_v.png"
                fillMode: Image.PreserveAspectFit
            }

            // 水平遮罩
            Image {
                id: maskHImage
                anchors.fill: parent
                source: "qrc:/Images/AC/mask_h.png"
                fillMode: Image.PreserveAspectFit
            }
        }
    }
    // 淡入动画效果
    PropertyAnimation {
        id: fadeInAnimation
        target: parent
        properties: "opacity"
        duration: 500
        from: 0
        to: 1
        easing.type: Easing.OutQuad
    }

    Component.onCompleted: fadeInAnimation.start()

    // 页面公共区域：状态栏、底部空调栏和自适应风量弹窗
    PageChrome {
        anchors.fill: parent
    }


    // 空调功能栏
    ACFunctionBar {
        id: acFunctionBar
        width: 882
        height: 70
        anchors.left: parent.left
        anchors.leftMargin: 213
        anchors.top: parent.top
        anchors.topMargin: 57

        onFunctionActivated: function(value) {
            if (value === 0) {
                acModePanel.close()
            } else if (value === 1) {
                acModePanel.close()
                Ui.showToast(qsTr("通风加热功能已切换"))
            } else if (value === 2) {
                acModePanel.close()
                Ui.showToast(qsTr("空气滤净功能已切换"))
            } else if (value === 3) {
                acModePanel.open()
            }
        }
    }

    // 左温度背景
    Image {
        id: leftTemperatureImage
        width: 270
        height: 511
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.top: parent.top
        anchors.topMargin: 158
        source: "qrc:/Images/AC/left_temperatur_background.png"
        fillMode: Image.PreserveAspectFit
    }

    // 左温度列表
    QuickTemperatureList {
        id: leftTemperatureList
        width: 160
        height: 511
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.top: parent.top
        anchors.topMargin: 158
        spacing: 10
        background: "transparent"
        currentIndexTextColor: "#04FAFB"
        otherIndexTextColor: "#DFDFDF"
        movingColor: "#DFDFDF"
        fontPixelSize: 38
        fontBold: true
        temperature: Ui.acLeftTemperature
        direction: 0

        onMovementStarted: {
            // console.log("onMovementStarted")
        }

        onMovementEnded: function(temperatureValue) {
            // console.log("temperatureValue: " + temperatureValue)
            Ui.acLeftTemperature = temperatureValue
        }
    }

    // 右温度背景
    Image {
        id: rightTemperatureImage
        width: 270
        height: 511
        anchors.left: parent.left
        anchors.leftMargin: 1047
        anchors.top: parent.top
        anchors.topMargin: 158
        source: "qrc:/Images/AC/right_temperatur_background.png"
        fillMode: Image.PreserveAspectFit
    }

    // 右温度列表
    QuickTemperatureList {
        id: rightTemperatureList
        width: 160
        height: 511
        anchors.left: parent.left
        anchors.leftMargin: 1157
        anchors.top: parent.top
        anchors.topMargin: 158
        spacing: 10
        background: "transparent"
        currentIndexTextColor: "#04FAFB"
        otherIndexTextColor: "#DFDFDF"
        movingColor: "#DFDFDF"
        fontPixelSize: 38
        fontBold: true
        temperature: Ui.acRightTemperature
        direction: 1

        onMovementStarted: {
            // console.log("onMovementStarted")
        }

        onMovementEnded: function(temperatureValue) {
            Ui.acRightTemperature = temperatureValue
        }
    }
    // 负离子按钮
    Button {
        id: anionButton
        width: 97
        height: 97
        anchors.left: parent.left
        anchors.leftMargin: 266
        anchors.top: parent.top
        anchors.topMargin: 368
        hoverEnabled: false
        z: leftRect.z + 2

        property bool switchStatus: true

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: parent.switchStatus ?
                "qrc:/Images/AC/function_on.png" :
                "qrc:/Images/AC/function_off.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        Label {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("负离子")
            color: "#FFFFFF"
            font.pixelSize: 18
        }

        onClicked: {
            switchStatus = !switchStatus
        }
    }

    // 香薰按钮
    Button {
        id: fragranceButton
        width: 97
        height: 97
        anchors.left: parent.left
        anchors.leftMargin: 1009
        anchors.top: parent.top
        anchors.topMargin: 368
        hoverEnabled: false
        z: rightRect.z + 1

        property bool switchStatus: true

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: parent.switchStatus ?
                "qrc:/Images/AC/function_on.png" :
                "qrc:/Images/AC/function_off.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        Label {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("香薰")
            color: "#FFFFFF"
            font.pixelSize: 18
        }

        onClicked: {
            switchStatus = !switchStatus
        }
    }
    // 右侧风向调节
    Rectangle {
        id: rightRect
        width: 340
        height: 300
        x: 800
        y: 200
        z: backgroundImage.z + 1
        // color: "#2011AA11"
        color: "transparent"
        // visible: false

        MouseArea {
            id: rightMouseArea
            anchors.fill: parent

            onPositionChanged: {
                if(mouse.x < 0)
                {
                    rightPercentX = 0
                }
                else if(mouse.x > rightMouseArea.width)
                {
                    rightPercentX = 100
                }
                else
                {
                    rightPercentX = mouse.x / xStepSize
                }

                if(mouse.y < 0)
                {
                    rightPercentY = 0
                }
                else if(mouse.y > rightMouseArea.height)
                {
                    rightPercentY = 100
                }
                else
                {
                    rightPercentY = mouse.y / yStepSize
                }
            }
        }
    }
    // 右侧风量显示
    QuickWind {
        id: rightWind
        width: 240
        height: 200
        emitterWidth: 360
        emitterHeight: 30
        source: "qrc:/Images/AC/fog.png"
        x: 890
        y: 210
        z: rightRect.z + 1
        moveX: rightPercentX
        moveY: rightPercentY
        offsetX: 0
        offsetY: 100
        value: Ui.acFanLevel
        color: getWindColor(Ui.acRightTemperature)
    }

    // 左侧风向调节
    Rectangle {
        id: leftRect
        width: 340
        height: 300
        x: 274
        y: 200
        z: backgroundImage.z + 1
        color: "transparent"

        MouseArea {
            id: leftMouseArea
            anchors.fill: parent

            onPositionChanged: {
                if(mouse.x < 0)
                {
                    leftPercentX = 0
                }
                else if(mouse.x > leftMouseArea.width)
                {
                    leftPercentX = 100
                }
                else
                {
                    leftPercentX = mouse.x / xStepSize
                }

                if(mouse.y < 0)
                {
                    leftPercentY = 0
                }
                else if(mouse.y > leftMouseArea.height)
                {
                    leftPercentY = 100
                }
                else
                {
                    leftPercentY = mouse.y / yStepSize
                }
            }
        }
    }

    // 左侧风量显示
    QuickWind {
        id: leftWind
        width: 240
        height: 200
        emitterWidth: 360
        emitterHeight: 30
        source: "qrc:/Images/AC/fog.png"
        x: 284
        y: 210
        z: leftRect.z + 1
        moveX: leftPercentX
        moveY: leftPercentY
        offsetX: 0
        offsetY: 100
        value: Ui.acFanLevel
        color: getWindColor(Ui.acLeftTemperature)
    }

    ACModePanel {
        id: acModePanel
    }

}
