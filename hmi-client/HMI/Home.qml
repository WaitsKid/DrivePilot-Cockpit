import QtQuick
import QtQuick.Controls
Item{
    width:1414
    height:856
    x:108
    y:0
    Image{
        id:backgroundImage
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode:Image.Stretch
    }
    //关联时间
    Connections {
        target: Ui

        function onUpdateDateTime(date, time) {
            timeLabel.text = time
            dateLabel.text=date
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
    // 时间
    Label {
        id: timeLabel
        width: 162
        height: 91
        anchors.left: parent.left
        anchors.leftMargin: 1217
        anchors.top: parent.top
        anchors.topMargin: 61
        text: qsTr("")
        color: "#FFFFFF"
        font.pixelSize: 64

    }

    // 日期
    Label {
        id: dateLabel
        width: 162
        height: 26
        anchors.left: parent.left
        anchors.leftMargin: 1217
        anchors.top: parent.top
        anchors.topMargin: 146
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("")
        color: "#9AFFFFFF"
        font.pixelSize: 18
    }
    //地图按钮
    Button{
        id:mapButton
        width: 310
        height: 387
        anchors.left: parent.left
        anchors.leftMargin: 79
        anchors.top: parent.top
        anchors.topMargin: 235
        hoverEnabled: false
        background: Image {
            width:parent.width
            height:parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/map.png"
            fillMode: Image.Stretch
            opacity:parent.down?0.6:1
            z:parent.z+1
            Image {
                width: 279
                height: 97
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 266
                source: "qrc:/Images/Home/map_inner.png"
                fillMode: Image.Stretch
                z: parent.z + 1

                // 回家
                Button {
                    id: mapHomeButton
                    width: 43
                    height: 55
                    anchors.left: parent.left
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Image {
                        width: 29
                        height: 54
                        anchors.centerIn: parent
                        source: "qrc:/Images/Home/map_home.png"
                        fillMode: Image.Stretch
                        opacity: parent.down ? 0.6 : 1
                    }

                    onClicked: Ui.navigateTo(Ui.PAGE_MAP)
                }

                // 去公司
                Button {
                    id: mapCompanyButton
                    width: 43
                    height: 55
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Image {
                        width: 43
                        height: 55
                        anchors.centerIn: parent
                        source: "qrc:/Images/Home/map_company.png"
                        fillMode: Image.Stretch
                        opacity: parent.down ? 0.6 : 1
                    }

                    onClicked: Ui.navigateTo(Ui.PAGE_MAP)
                }

                // 充电站
                Button {
                    id: mapChargingStationButton
                    width: 43
                    height: 55
                    anchors.right: parent.right
                    anchors.rightMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Image {
                        width: 43
                        height: 55
                        anchors.centerIn: parent
                        source: "qrc:/Images/Home/map_charging_station.png"
                        fillMode: Image.Stretch
                        opacity: parent.down ? 0.6 : 1
                    }

                    onClicked: Ui.navigateTo(Ui.PAGE_MAP)
                }
            }
        }
        onClicked: Ui.navigateTo(Ui.PAGE_MAP)
    }
    // 音乐按钮
    Button {
        id: musicButton
        width: 310
        height: 387
        anchors.left: parent.left
        anchors.leftMargin: 417
        anchors.top: parent.top
        anchors.topMargin: 235
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/music.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
            z: parent.z + 1

            // 当前歌曲封面由 QML 图形动态绘制
            MusicCover {
                id: musicAlbumImage
                width: 104
                height: 104
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 74
                primaryColor: MusicPlayer.primaryColor
                secondaryColor: MusicPlayer.secondaryColor
                title: MusicPlayer.title
                variant: MusicPlayer.coverVariant
                playing: MusicPlayer.playing
                cornerRadius: 20
            }

            // 歌词
            Label {
                id: lyricLabel
                width: parent.width
                height: 26
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 197
                text: MusicPlayer.title
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#FFFFFF"
                font.pixelSize: 18
                font.bold: true
            }

            // 艺术家
            Label {
                id: artistLabel
                width: parent.width
                height: 26
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 227
                text: MusicPlayer.artist
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#9AFFFFFF"
                font.pixelSize: 16
            }


            // 播放控制背景
            Image {
                width: 279 * 1.15
                height: 97 * 1.15
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 266 - 15
                source: "qrc:/Images/Home/music_inner.png"
                fillMode: Image.PreserveAspectFit
                z: parent.z + 1

                // 上一曲
                Button {
                    id: musicPreviousButton
                    width: 43
                    height: 55
                    anchors.left: parent.left
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: 7
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Image {
                        width: 19
                        height: 20
                        anchors.centerIn: parent
                        source: "qrc:/Images/Home/music_previous.png"
                        fillMode: Image.PreserveAspectFit
                        opacity: parent.down ? 0.6 : 1
                    }

                    onClicked: MusicPlayer.previous()
                }

                // 播放
                Button {
                    id: musicPlayButton
                    width: 43
                    height: 55
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: 7
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Rectangle {
                        width: 38
                        height: 38
                        radius: 19
                        anchors.centerIn: parent
                        color: parent.down ? "#3AFFFFFF" : "#20FFFFFF"

                        Label {
                            anchors.centerIn: parent
                            text: MusicPlayer.playing ? "Ⅱ" : "▶"
                            color: "#FFFFFF"
                            font.pixelSize: 19
                            font.bold: true
                        }
                    }

                    onClicked: MusicPlayer.playPause()
                }

                // 下一曲
                Button {
                    id: musicNextButton
                    width: 43
                    height: 55
                    anchors.right: parent.right
                    anchors.rightMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: 7
                    hoverEnabled: false
                    z: parent.z + 1

                    background: Image {
                        width: 19
                        height: 20
                        anchors.centerIn: parent
                        source: "qrc:/Images/Home/music_next.png"
                        fillMode: Image.PreserveAspectFit
                        opacity: parent.down ? 0.6 : 1
                    }

                    onClicked: MusicPlayer.next()
                }
            }
        }

        onClicked: Ui.navigateTo(Ui.PAGE_MUSIC)
    }
    // 车况按钮
    Button {
        id: vehicleConditionButton
        width: 624
        height: 177
        anchors.left: parent.left
        anchors.leftMargin: 756
        anchors.top: parent.top
        anchors.topMargin: 235
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            // anchors.centerIn: parent
            source: "qrc:/Images/Home/vehicle_condition.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: {
            Ui.navigateTo(Ui.PAGE_VEHICLE)
        }

        // 天数
        Label {
            id: daysLabel1
            width: 90
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 23
            anchors.top: parent.top
            anchors.topMargin: 18
            verticalAlignment: Text.AlignVCenter
            text: qsTr("已安全陪伴您 ")
            color: "#9AFFFFFF"
            font.pixelSize: 16
        }
        Label {
            id: daysLabel
            width: 50
            height: 32
            anchors.left: daysLabel1.right
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 13
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("267")
            color: "#FFFFFF"
            font.pixelSize: 24
            font.bold: true
        }
        Label {
            id: daysLabel2
            width: 32
            height: 26
            anchors.left: daysLabel.right
            anchors.leftMargin: 5
            anchors.top: parent.top
            anchors.topMargin: 18
            verticalAlignment: Text.AlignVCenter
            text: qsTr(" 天")
            color: "#9AFFFFFF"
            font.pixelSize: 16
        }


        // 车
        Image {
            width: 336
            height: 129
            anchors.left: parent.left
            anchors.leftMargin: 306
            anchors.top: parent.top
            anchors.topMargin: 40
            source: "qrc:/Images/Home/vehicle.png"
            fillMode: Image.PreserveAspectFit
        }

        // 车况
        Rectangle {
            width: 126
            height: 31
            radius: 15
            anchors.left: parent.left
            anchors.leftMargin: 244
            anchors.top: parent.top
            anchors.topMargin: 16
            color: VehicleData.tireFaultActive ? "#35FF6570" : "#243ECDA5"

            Label {
                anchors.centerIn: parent
                text: VehicleData.healthText
                color: VehicleData.tireFaultActive ? "#FF737C" : "#54E3C2"
                font.pixelSize: 13
                font.bold: true
            }
        }

        // 里程
        Column {
            anchors.left: parent.left
            anchors.leftMargin: 23
            anchors.top: parent.top
            anchors.topMargin: 80
            spacing: 1

            Label {
                text: qsTr("剩余续航")
                color: "#8FFFFFFF"
                font.pixelSize: 14
            }

            Row {
                spacing: 5

                Label {
                    text: VehicleData.remainingRange
                    color: "#FFFFFF"
                    font.pixelSize: 30
                    font.bold: true
                }

                Label {
                            text: "km"
                    color: "#9AFFFFFF"
                    font.pixelSize: 14
                }
            }

            Label {
                text: qsTr("电量 %1% · 点击查看能耗与健康").arg(VehicleData.batteryLevel)
                color: "#68DCC5"
                font.pixelSize: 12
            }
        }
    }
    // 收音机按钮
    Button {
        id: radioButton
        width: 414
        height: 177
        anchors.left: parent.left
        anchors.leftMargin: 756
        anchors.top: parent.top
        anchors.topMargin: 446
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/radio_background.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1

            // 收音机Logo
            Image {
                id: radioLogoImage
                width: 93
                height: 29
                anchors.left: parent.left
                anchors.leftMargin: 23
                anchors.top: parent.top
                anchors.topMargin: 20
                source: "qrc:/Images/Home/radio_logo.png"
                fillMode: Image.PreserveAspectFit
            }

            // 收音机Logo
            Image {
                id: radioSloganImage
                width: 131
                height: 59
                anchors.left: parent.left
                anchors.leftMargin: 23
                anchors.top: parent.top
                anchors.topMargin: 92
                source: "qrc:/Images/Home/radio_slogan.png"
                fillMode: Image.PreserveAspectFit
            }

            // 收音机
            Image {
                id: radioImage
                width: 120
                height: 120
                anchors.left: parent.left
                anchors.leftMargin: 271
                anchors.top: parent.top
                anchors.topMargin: 31
                source: "qrc:/Images/Home/radio.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        onClicked: Ui.showToast(qsTr("收音机功能暂未接入"))
    }
    // 应用按钮
    Button {
        id: appButton
        width: 220
        height: 217
        anchors.left: parent.left
        anchors.leftMargin: 1159
        anchors.top: parent.top
        anchors.topMargin: 439
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/app.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: {
            Ui.navigateTo(Ui.PAGE_APP)
        }
    }
    // 天气按钮
    Button {
        id: weatherButton
        width: 627
        height: 120
        anchors.left: parent.left
        anchors.leftMargin: 551
        anchors.top: parent.top
        anchors.topMargin: 67
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/weather_background.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1

            WeatherIcon {
                width: 82
                height: 72
                anchors.left: parent.left
                anchors.leftMargin: 41
                anchors.top: parent.top
                anchors.topMargin: 24
                weatherCode: Weather.weatherCode
                sourcePixelSize: 180
                accessibleName: Weather.condition
            }
        }

        // 区域
        Label {
            id: regionLabel
            width: 135
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 172
            anchors.top: parent.top
            anchors.topMargin: 30
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: Weather.locationName
            color: "#FFFFFF"
            font.pixelSize: 18
        }

        // 状态
        Label {
            id: weatherLabel
            width: 135
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 172
            anchors.top: parent.top
            anchors.topMargin: 64
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: Weather.loading ? qsTr("天气更新中…") : Weather.condition + " " + Weather.temperature + "°"
            color: "#9AFFFFFF"
            font.pixelSize: 18
        }

        // 分割线
        Rectangle {
            width: 3
            height: 55
            anchors.left: parent.left
            anchors.leftMargin: 356
            anchors.top: parent.top
            anchors.topMargin: 31
            color: "#2C333E"
        }

        // 空气质量提示
        Label {
            id: airQualityTipsLabel
            anchors.left: parent.left
            anchors.leftMargin: 412
            anchors.top: parent.top
            anchors.topMargin: 30
            text: qsTr("空气质量")
            color: "#FFFFFF"
            font.pixelSize: 18
        }

        // 空气质量
        Label {
            id: airQualityLabel
            anchors.left: parent.left
            anchors.leftMargin: 494
            anchors.top: parent.top
            anchors.topMargin: 30
            text: Weather.airQualityLevel
            color: Weather.airQualityColor
            font.pixelSize: 18
        }

        // 温度
        Label {
            id: temperatureLabel
            anchors.left: parent.left
            anchors.leftMargin: 412
            anchors.top: parent.top
            anchors.topMargin: 64
            text: qsTr("车内 %1°  车外 %2°").arg(Math.round((Ui.acLeftTemperature + Ui.acRightTemperature) / 2)).arg(Weather.temperature)
            color: "#9AFFFFFF"
            font.pixelSize: 18
        }

        onClicked: Ui.navigateTo(Ui.PAGE_WEATHER)
    }
    // 语音助手按钮
    Button {
        id: voiceAssistantButton
        width: 444
        height: 120
        anchors.left: parent.left
        anchors.leftMargin: 79
        anchors.top: parent.top
        anchors.topMargin: 67
        hoverEnabled: false

        background: Image {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            source: "qrc:/Images/Home/voice_assistant_background.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1

            Image {
                id: voiceAssistantImage
                width: 86
                height: 86
                anchors.left: parent.left
                anchors.leftMargin: 17
                anchors.top: parent.top
                anchors.topMargin: 20
                source: "qrc:/Images/Home/voice_assistant.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        Label {
            id: voiceTipsLabel
            width: 220
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 118
            anchors.top: parent.top
            anchors.topMargin: 30
            text: qsTr("你可以这样说：")
            color: "#9AFFFFFF"
            font.pixelSize: 16
        }

        Label {
            id: voiceAssitantLabel
            width: 220
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 118
            anchors.top: parent.top
            anchors.topMargin: 63
            text: qsTr("小迪去公司的路况怎么样？")
            color: "#FFFFFF"
            font.pixelSize: 18
        }

        onClicked: Ui.navigateTo(Ui.PAGE_ASSISTANT)
    }

    // 页面公共区域：状态栏、底部空调栏和自适应风量弹窗
    PageChrome {
        anchors.fill: parent
    }


}
