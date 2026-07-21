import QtQuick
import QtQuick.Controls
Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0
    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#28091420"
    }

    PropertyAnimation {
        id: fadeInAnimation
        target: root
        property: "opacity"
        duration: 420
        from: 0
        to: 1
        easing.type: Easing.OutCubic
    }

    Component.onCompleted: fadeInAnimation.start()

    

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 55
        anchors.top: parent.top
        anchors.topMargin: 62
        text: qsTr("车辆能耗与健康")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.bold: true
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 57
        anchors.top: parent.top
        anchors.topMargin: 102
        text: qsTr("C++ 实时模拟 · 胎压监测 · 能量流 · 驾驶模式")
        color: "#7FFFFFFF"
        font.pixelSize: 14
    }

    Button {
        width: 150
        height: 42
        anchors.right: parent.right
        anchors.rightMargin: 120
        anchors.top: parent.top
        anchors.topMargin: 76
        hoverEnabled: false

        contentItem: Label {
            text: qsTr("进入车辆设置")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "#EFFFFFFF"
            font.pixelSize: 14
            font.bold: true
        }

        background: Rectangle {
            radius: 14
            color: parent.down ? "#385A78" : "#293E54"
            border.width: 1
            border.color: "#4E7191"
        }

        onClicked: Ui.navigateTo(Ui.PAGE_SETTINGS)
    }

    Rectangle {
        id: batteryPanel
        width: 330
        height: 540
        anchors.left: parent.left
        anchors.leftMargin: 55
        anchors.top: parent.top
        anchors.topMargin: 137
        radius: 26
        color: "#5A111B29"
        border.width: 1
        border.color: "#2EFFFFFF"

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("动力电池")
            color: "#FFFFFF"
            font.pixelSize: 20
            font.bold: true
        }

        Rectangle {
            width: 78
            height: 30
            radius: 15
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 17
            color: VehicleData.batteryLevel > 25 ? "#243ECDA5" : "#35FF6570"

            Label {
                anchors.centerIn: parent
                text: VehicleData.batteryLevel > 25 ? qsTr("状态正常") : qsTr("电量偏低")
                color: VehicleData.batteryLevel > 25 ? "#54E3C2" : "#FF737C"
                font.pixelSize: 12
                font.bold: true
            }
        }

        CircularGauge {
            width: 244
            height: 244
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 58
            value: VehicleData.batteryLevel
            minimumValue: 0
            maximumValue: 100
            title: qsTr("剩余电量")
            unit: "%"
            progressColor: VehicleData.batteryLevel > 25 ? "#48E5C2" : "#FF6873"
        }

        Rectangle {
            width: parent.width - 42
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 316
            color: "#20FFFFFF"
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 336
            spacing: 44

            Column {
                spacing: 4

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: VehicleData.remainingRange
                    color: "#FFFFFF"
                    font.pixelSize: 31
                    font.bold: true
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("剩余续航 km")
                    color: "#78FFFFFF"
                    font.pixelSize: 13
                }
            }

            Rectangle {
                width: 1
                height: 58
                color: "#28FFFFFF"
            }

            Column {
                spacing: 4

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: VehicleData.speed
                    color: "#FFFFFF"
                    font.pixelSize: 31
                    font.bold: true
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("当前车速 km/h")
                    color: "#78FFFFFF"
                    font.pixelSize: 13
                }
            }
        }

        Button {
            width: 286
            height: 54
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24
            hoverEnabled: false

            contentItem: Label {
                text: VehicleData.simulationRunning ? qsTr("暂停实时数据") : qsTr("继续实时数据")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#FFFFFF"
                font.pixelSize: 15
                font.bold: true
            }

            background: Rectangle {
                radius: 16
                color: parent.down ? "#345C85" : "#294866"
                border.width: 1
                border.color: "#46779E"
            }

            onClicked: {
                VehicleData.simulationRunning = !VehicleData.simulationRunning
                Ui.showToast(VehicleData.simulationRunning
                             ? qsTr("车辆实时数据已恢复")
                             : qsTr("车辆实时数据已暂停"))
            }
        }
    }

    Rectangle {
        id: energyPanel
        width: 560
        height: 540
        anchors.left: batteryPanel.right
        anchors.leftMargin: 25
        anchors.top: batteryPanel.top
        radius: 26
        color: "#52111B29"
        border.width: 1
        border.color: "#2EFFFFFF"

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("整车能量流")
            color: "#FFFFFF"
            font.pixelSize: 20
            font.bold: true
        }

        Rectangle {
            width: 126
            height: 30
            radius: 15
            anchors.right: parent.right
            anchors.rightMargin: 22
            anchors.top: parent.top
            anchors.topMargin: 17
            color: VehicleData.energyFlowState === VehicleData.FLOW_REGEN
                   ? "#2444E4A6"
                   : (VehicleData.energyFlowState === VehicleData.FLOW_DRIVE
                      ? "#254D8DFF" : "#24FFFFFF")

            Label {
                anchors.centerIn: parent
                text: VehicleData.energyFlowState === VehicleData.FLOW_REGEN
                      ? qsTr("能量回收中")
                      : (VehicleData.energyFlowState === VehicleData.FLOW_DRIVE
                         ? qsTr("车辆驱动中") : qsTr("驻车状态"))
                color: VehicleData.energyFlowState === VehicleData.FLOW_REGEN
                       ? "#55E5B3"
                       : (VehicleData.energyFlowState === VehicleData.FLOW_DRIVE
                          ? "#73A8FF" : "#A0FFFFFF")
                font.pixelSize: 12
                font.bold: true
            }
        }

        EnergyFlow {
            width: 500
            height: 374
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 60
            flowState: VehicleData.energyFlowState
            idleState: VehicleData.FLOW_IDLE
            driveState: VehicleData.FLOW_DRIVE
            regenState: VehicleData.FLOW_REGEN
        }

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 27
            anchors.top: parent.top
            anchors.topMargin: 434
            text: qsTr("驾驶模式")
            color: "#8FFFFFFF"
            font.pixelSize: 14
        }

        Label {
            anchors.right: parent.right
            anchors.rightMargin: 27
            anchors.top: parent.top
            anchors.topMargin: 434
            text: VehicleData.driveModeText
            color: "#FFFFFF"
            font.pixelSize: 14
            font.bold: true
        }

        DriveModeSelector {
            width: 506
            height: 58
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 22
            currentMode: VehicleData.driveMode

            onModeSelected: function(mode) {
                VehicleData.driveMode = mode
                Ui.showToast(qsTr("已切换至%1").arg(VehicleData.driveModeText))
            }
        }
    }

    Rectangle {
        id: healthPanel
        width: 365
        height: 540
        anchors.left: energyPanel.right
        anchors.leftMargin: 25
        anchors.top: batteryPanel.top
        radius: 26
        color: "#5A111B29"
        border.width: 1
        border.color: VehicleData.tireFaultActive ? "#78FF6570" : "#2EFFFFFF"

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("车辆健康")
            color: "#FFFFFF"
            font.pixelSize: 20
            font.bold: true
        }

        Rectangle {
            width: 88
            height: 30
            radius: 15
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 17
            color: VehicleData.tireFaultActive ? "#35FF6570" : "#243ECDA5"

            Label {
                anchors.centerIn: parent
                text: VehicleData.healthText
                color: VehicleData.tireFaultActive ? "#FF737C" : "#54E3C2"
                font.pixelSize: 12
                font.bold: true
            }
        }

        Grid {
            id: metricGrid
            columns: 2
            spacing: 10
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 62

            MetricCard {
                width: 157
                height: 91
                title: qsTr("瞬时功率")
                displayValue: VehicleData.instantPower.toFixed(1)
                unit: "kW"
                detail: VehicleData.instantPower < 0 ? qsTr("回收") : qsTr("输出")
                accentColor: VehicleData.instantPower < 0 ? "#45E3A9" : "#5A91FF"
            }

            MetricCard {
                width: 157
                height: 91
                title: qsTr("平均能耗")
                displayValue: VehicleData.averageConsumption.toFixed(1)
                unit: "kWh/100km"
                detail: qsTr("近 50 km")
                accentColor: "#CE8DFF"
            }

            MetricCard {
                width: 157
                height: 91
                title: qsTr("电池温度")
                displayValue: VehicleData.batteryTemperature.toFixed(1)
                unit: "℃"
                detail: qsTr("热管理正常")
                accentColor: "#52D9C4"
            }

            MetricCard {
                width: 157
                height: 91
                title: qsTr("电机温度")
                displayValue: VehicleData.motorTemperature.toFixed(1)
                unit: "℃"
                detail: qsTr("工作区间")
                accentColor: "#F0A66A"
            }
        }

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.top: parent.top
            anchors.topMargin: 262
            text: qsTr("四轮胎压")
            color: "#8FFFFFFF"
            font.pixelSize: 14
        }

        Grid {
            columns: 2
            spacing: 10
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 288

            TirePressureCard {
                width: 157
                height: 82
                positionText: qsTr("左前")
                pressure: VehicleData.frontLeftPressure
            }

            TirePressureCard {
                width: 157
                height: 82
                positionText: qsTr("右前")
                pressure: VehicleData.frontRightPressure
            }

            TirePressureCard {
                width: 157
                height: 82
                positionText: qsTr("左后")
                pressure: VehicleData.rearLeftPressure
            }

            TirePressureCard {
                width: 157
                height: 82
                positionText: qsTr("右后")
                pressure: VehicleData.rearRightPressure
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            spacing: 10

            Button {
                width: 157
                height: 48
                hoverEnabled: false

                contentItem: Label {
                    text: VehicleData.tireFaultActive ? qsTr("恢复胎压") : qsTr("模拟胎压故障")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                }

                background: Rectangle {
                    radius: 15
                    color: VehicleData.tireFaultActive
                           ? (parent.down ? "#268B75" : "#23725F")
                           : (parent.down ? "#9A3E49" : "#7C343D")
                    border.width: 1
                    border.color: VehicleData.tireFaultActive ? "#42C8A7" : "#D15D68"
                }

                onClicked: {
                    if (VehicleData.tireFaultActive) {
                        VehicleData.clearTireFault()
                        Ui.showToast(qsTr("胎压已恢复，车辆健康状态正常"))
                    } else {
                        VehicleData.simulateTireFault()
                        Ui.showToast(qsTr("已模拟右前轮低胎压故障"))
                    }
                }
            }

            Button {
                width: 157
                height: 48
                hoverEnabled: false

                contentItem: Label {
                    text: qsTr("重置数据")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#DFFFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                }

                background: Rectangle {
                    radius: 15
                    color: parent.down ? "#36475C" : "#293747"
                    border.width: 1
                    border.color: "#50677F"
                }

                onClicked: {
                    VehicleData.resetSimulation()
                    Ui.showToast(qsTr("车辆模拟数据已恢复默认值"))
                }
            }
        }
    }

        // 页面公共区域：状态栏、底部空调栏和自适应风量弹窗
    PageChrome {
        anchors.fill: parent
    }

}
