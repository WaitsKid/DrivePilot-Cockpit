import QtQuick
import QtQuick.Controls
import BYD

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0

    property bool manualRequest: false
    property bool suggestionPanelOpen: false
    property int highlightedSuggestion: -1

    function chooseSuggestion(index) {
        if (index < 0 || index >= cityList.count)
            return

        manualRequest = true
        suggestionPanelOpen = false
        highlightedSuggestion = -1
        Weather.selectCitySuggestion(index)
        searchField.focus = false
    }

    function requestCity() {
        if (suggestionPanelOpen && highlightedSuggestion >= 0) {
            chooseSuggestion(highlightedSuggestion)
            return
        }

        const query = searchField.text.trim()
        if (query.length === 0) {
            suggestionPanelOpen = true
            Weather.showDefaultCitySuggestions()
            Ui.showToast(qsTr("可以直接从热门城市或最近使用中选择"))
            return
        }

        manualRequest = true
        suggestionPanelOpen = false
        Weather.searchLocation(query)
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#26071018"
    }

    Component.onCompleted: {
        searchField.text = Weather.locationQuery
        // 避免在页面对象树尚未完成构建时同步重置候选模型。
        Qt.callLater(Weather.showDefaultCitySuggestions)
    }

    Connections {
        target: Weather

        function onLocationQueryChanged() {
            if (!searchField.activeFocus)
                searchField.text = Weather.locationQuery
        }

        function onRefreshFinished(success, message) {
            if (root.manualRequest) {
                Ui.showToast(message)
                root.manualRequest = false
            }
        }
    }

    Text {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 54
        anchors.top: parent.top
        anchors.topMargin: 61
        text: qsTr("天气详情")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Row {
        id: statusRow
        anchors.left: titleLabel.right
        anchors.leftMargin: 20
        anchors.verticalCenter: titleLabel.verticalCenter
        spacing: 9

        Rectangle {
            width: 9
            height: 9
            radius: 5
            anchors.verticalCenter: parent.verticalCenter
            color: Weather.online ? "#54D6A6" : (Weather.usingCache ? "#F1D16A" : "#8B9AAF")
        }

        Text {
            text: Weather.dataSource + " · " + Weather.lastUpdated
            color: "#82FFFFFF"
            font.pixelSize: 13
        }
    }

    Timer {
        id: suggestionTimer
        interval: 300
        repeat: false
        onTriggered: Weather.requestCitySuggestions(searchField.text)
    }

    Timer {
        id: suggestionCloseTimer
        interval: 180
        repeat: false
        onTriggered: root.suggestionPanelOpen = false
    }

    // 下拉框打开时覆盖页面其余区域：点击任意空白处即可关闭。
    // 搜索框和下拉面板的 z 值更高，因此它们自身仍能正常交互。
    MouseArea {
        id: suggestionDismissArea
        anchors.fill: parent
        visible: root.suggestionPanelOpen
        enabled: visible
        z: 1100
        acceptedButtons: Qt.LeftButton

        onClicked: {
            suggestionTimer.stop()
            suggestionCloseTimer.stop()
            root.suggestionPanelOpen = false
            root.highlightedSuggestion = -1
            searchField.focus = false
        }
    }

    Text {
        anchors.left: statusRow.left
        anchors.top: titleLabel.bottom
        anchors.topMargin: 4
        text: qsTr("位置来源：") + Weather.locationMethod
        color: Weather.locating ? "#7DB9FF" : "#5FFFFFFF"
        font.pixelSize: 11
    }

    TextField {
        id: searchField
        width: 232
        height: 44
        anchors.right: searchButton.left
        anchors.rightMargin: 10
        anchors.verticalCenter: titleLabel.verticalCenter
        placeholderText: qsTr("输入中国省 / 市 / 区县 / 街道")
        color: "#FFFFFF"
        placeholderTextColor: "#62FFFFFF"
        font.pixelSize: 15
        leftPadding: 17
        rightPadding: 38
        selectByMouse: true
        verticalAlignment: Text.AlignVCenter
        z: 1200

        background: Rectangle {
            radius: 22
            color: searchField.activeFocus ? "#273548" : "#1D2836"
            border.width: 1
            border.color: searchField.activeFocus ? "#6FA8F6" : "#24FFFFFF"
        }

        Rectangle {
            width: 31
            height: 31
            radius: 16
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            color: cityMenuMouse.containsMouse ? "#274762" : "transparent"

            Text {
                anchors.centerIn: parent
                text: "⌄"
                color: "#AFFFFFFF"
                font.pixelSize: 18
            }

            MouseArea {
                id: cityMenuMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    suggestionTimer.stop()
                    suggestionCloseTimer.stop()
                    root.highlightedSuggestion = -1

                    if (root.suggestionPanelOpen) {
                        root.suggestionPanelOpen = false
                        searchField.focus = false
                        return
                    }

                    root.suggestionPanelOpen = true
                    Weather.showDefaultCitySuggestions()
                }
            }
        }

        onTextEdited: {
            root.suggestionPanelOpen = true
            root.highlightedSuggestion = -1
            suggestionTimer.restart()
        }

        onActiveFocusChanged: {
            if (activeFocus) {
                suggestionCloseTimer.stop()
                root.suggestionPanelOpen = true
                root.highlightedSuggestion = -1
                Weather.requestCitySuggestions(text)
            } else {
                suggestionCloseTimer.restart()
            }
        }

        onAccepted: root.requestCity()

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Down) {
                root.suggestionPanelOpen = true
                root.highlightedSuggestion = Math.min(cityList.count - 1,
                    root.highlightedSuggestion + 1)
                if (root.highlightedSuggestion >= 0)
                    cityList.positionViewAtIndex(root.highlightedSuggestion, ListView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                if (cityList.count > 0) {
                    root.highlightedSuggestion = Math.max(0,
                        root.highlightedSuggestion - 1)
                    cityList.positionViewAtIndex(root.highlightedSuggestion, ListView.Contain)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Escape) {
                root.suggestionPanelOpen = false
                root.highlightedSuggestion = -1
                event.accepted = true
            }
        }
    }

    Button {
        id: searchButton
        width: 72
        height: 44
        anchors.right: locateButton.left
        anchors.rightMargin: 10
        anchors.verticalCenter: titleLabel.verticalCenter
        hoverEnabled: false
        enabled: !Weather.loading && !Weather.locating
        z: 1200

        contentItem: Text {
            text: qsTr("查询")
            color: "#FFFFFF"
            font.pixelSize: 15
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 22
            color: searchButton.down ? "#38618F" : "#31547A"
            border.width: 1
            border.color: "#6FA8F6"
        }

        onClicked: root.requestCity()
    }

    Button {
        id: locateButton
        width: 108
        height: 44
        anchors.right: refreshButton.left
        anchors.rightMargin: 10
        anchors.verticalCenter: titleLabel.verticalCenter
        hoverEnabled: false
        enabled: !Weather.loading && !Weather.locating
        z: 1200

        contentItem: Text {
            text: Weather.locating ? qsTr("⌖  定位中") : qsTr("⌖  定位")
            color: "#FFFFFF"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 22
            color: locateButton.down ? "#285B60" : "#23484E"
            border.width: 1
            border.color: Weather.locating ? "#70D9C9" : "#397B7D"
        }

        onClicked: {
            root.manualRequest = true
            root.suggestionPanelOpen = false
            Weather.locateDevice()
        }
    }

    Button {
        id: refreshButton
        width: 92
        height: 44
        anchors.right: parent.right
        anchors.rightMargin: 108
        anchors.verticalCenter: titleLabel.verticalCenter
        hoverEnabled: false
        enabled: !Weather.loading && !Weather.locating
        z: 1200

        contentItem: Text {
            text: Weather.loading ? qsTr("… 更新中") : qsTr("↻  刷新")
            color: "#FFFFFF"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 22
            color: refreshButton.down ? "#344457" : "#263343"
            border.width: 1
            border.color: "#2FFFFFFF"
        }

        onClicked: {
            root.manualRequest = true
            root.suggestionPanelOpen = false
            root.highlightedSuggestion = -1
            Weather.refresh()
        }
    }

    Rectangle {
        id: suggestionPanel
        x: searchField.x
        y: searchField.y + searchField.height + 8
        width: 360
        height: Math.min(314, Math.max(74, cityList.contentHeight + 46))
        radius: 20
        color: "#F2172331"
        border.width: 1
        border.color: "#3BFFFFFF"
        visible: root.suggestionPanelOpen
        z: 1300
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: searchField.activeFocus ? 1 : 0
            border.color: "#356FA8F6"
        }

        Text {
            id: suggestionTitle
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 12
            text: searchField.text.trim().length === 0
                ? qsTr("最近使用与热门城市")
                : qsTr("高德中国行政区候选")
            color: "#AFFFFFFF"
            font.pixelSize: 12
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: suggestionTitle.verticalCenter
            visible: Weather.suggestionsLoading
            text: qsTr("查询中…")
            color: "#8FFFFFFF"
            font.pixelSize: 12
        }

        ListView {
            id: cityList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 38
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            clip: true
            spacing: 2
            model: Weather.citySuggestions
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: root.highlightedSuggestion

            delegate: CitySuggestionDelegate {
                width: cityList.width
                cityName: model.name
                detailText: model.detail
                sourceText: model.source
                levelText: model.level
                adcodeText: model.adcode
                highlighted: index === root.highlightedSuggestion

                onChosen: {
                    suggestionCloseTimer.stop()
                    root.chooseSuggestion(index)
                }
            }
        }

        Text {
            anchors.centerIn: cityList
            visible: !Weather.suggestionsLoading && cityList.count === 0
            text: qsTr("没有找到匹配的中国行政区")
            color: "#78FFFFFF"
            font.pixelSize: 13
        }
    }

    Rectangle {
        id: currentCard
        width: 360
        height: 274
        anchors.left: parent.left
        anchors.leftMargin: 54
        anchors.top: parent.top
        anchors.topMargin: 126
        radius: 26
        color: "#172333"
        border.width: 1
        border.color: "#2AFFFFFF"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 20
            text: Weather.locationName
            color: "#FFFFFF"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            width: 300
        }

        WeatherIcon {
            width: 76
            height: 76
            anchors.left: parent.left
            anchors.leftMargin: 21
            anchors.top: parent.top
            anchors.topMargin: 48
            weatherCode: Weather.weatherCode
            sourcePixelSize: 180
            accessibleName: Weather.condition
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 112
            anchors.top: parent.top
            anchors.topMargin: 52
            text: Weather.temperature + "°"
            color: "#FFFFFF"
            font.pixelSize: 65
            font.weight: Font.Light
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 25
            anchors.top: parent.top
            anchors.topMargin: 137
            text: Weather.condition + " · 体感 " + Weather.apparentTemperature + "°"
            color: "#CFFFFFFF"
            font.pixelSize: 17
        }

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 174
            height: 1
            color: "#22FFFFFF"
        }

        Grid {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 193
            columns: 2
            columnSpacing: 36
            rowSpacing: 12

            Text {
                text: "湿度  " + Weather.humidity + "%"
                color: "#AFFFFFFF"
                font.pixelSize: 14
            }
            Text {
                text: "降水  " + Weather.precipitationProbability + "%"
                color: "#AFFFFFFF"
                font.pixelSize: 14
            }
            Text {
                text: "风速  " + Weather.windSpeed + " km/h"
                color: "#AFFFFFFF"
                font.pixelSize: 14
            }
            Text {
                text: "能见度  " + Weather.visibility + " km"
                color: "#AFFFFFFF"
                font.pixelSize: 14
            }
        }
    }

    Rectangle {
        id: hourlyPanel
        anchors.left: currentCard.right
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 108
        anchors.top: currentCard.top
        height: currentCard.height
        radius: 26
        color: "#111B28"
        border.width: 1
        border.color: "#24FFFFFF"

        Text {
            id: hourlyTitle
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.top: parent.top
            anchors.topMargin: 17
            text: qsTr("未来 24 小时")
            color: "#FFFFFF"
            font.pixelSize: 19
            font.weight: Font.DemiBold
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 22
            anchors.verticalCenter: hourlyTitle.verticalCenter
            text: qsTr("横向滚动查看更多")
            color: "#60FFFFFF"
            font.pixelSize: 12
        }

        ListView {
            id: hourlyList
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 54
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            orientation: ListView.Horizontal
            spacing: 10
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: Weather.hourlyForecast

            delegate: HourlyWeatherDelegate {
                width: 104
                height: hourlyList.height
                timeText: model.timeText
                weatherCode: model.weatherCode
                condition: model.condition
                temperature: model.temperature
                precipitation: model.precipitation
                humidity: model.humidity
                currentHour: index === 0
            }
        }
    }

    Row {
        id: metricRow
        anchors.left: currentCard.left
        anchors.right: hourlyPanel.right
        anchors.top: currentCard.bottom
        anchors.topMargin: 18
        spacing: 14

        WeatherMetricCard {
            width: (metricRow.width - 42) / 4
            height: 92
            title: qsTr("空气质量")
            value: Weather.airQualityLevel + "  " + Weather.airQualityIndex
            detail: "PM2.5 " + Number(Weather.pm25).toFixed(1) + "  ·  PM10 " + Number(Weather.pm10).toFixed(1)
            iconText: "AQ"
            accentColor: Weather.airQualityColor
        }

        WeatherMetricCard {
            width: (metricRow.width - 42) / 4
            height: 92
            title: qsTr("体感温度")
            value: Weather.apparentTemperature + "°C"
            detail: Weather.comfortAdvice
            iconText: "℃"
            accentColor: "#F1B66A"
        }

        WeatherMetricCard {
            width: (metricRow.width - 42) / 4
            height: 92
            title: qsTr("风向风速")
            value: Weather.windSpeed + " km/h"
            detail: Weather.windDirectionName + "风 · 方向角 " + Weather.windDirection + "°"
            iconText: "↗"
            accentColor: "#75B9FF"
        }

        WeatherMetricCard {
            width: (metricRow.width - 42) / 4
            height: 92
            title: qsTr("驾驶建议")
            value: Weather.precipitationProbability >= 70 ? qsTr("谨慎驾驶") : qsTr("路况良好")
            detail: Weather.drivingAdvice
            iconText: "◇"
            accentColor: Weather.precipitationProbability >= 70 ? "#F3A45B" : "#54D6A6"
        }
    }

    Rectangle {
        id: dailyPanel
        anchors.left: currentCard.left
        anchors.right: hourlyPanel.right
        anchors.top: metricRow.bottom
        anchors.topMargin: 18
        height: 172
        radius: 24
        color: "#101925"
        border.width: 1
        border.color: "#22FFFFFF"

        Text {
            id: dailyTitle
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 14
            text: qsTr("未来 7 天")
            color: "#FFFFFF"
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }

        ListView {
            id: dailyList
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 45
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            orientation: ListView.Horizontal
            spacing: 10
            clip: true
            interactive: contentWidth > width
            boundsBehavior: Flickable.StopAtBounds
            model: Weather.dailyForecast

            delegate: DailyWeatherDelegate {
                width: (dailyList.width - 60) / 7
                height: dailyList.height
                weekdayText: model.weekdayText
                dateText: model.dateText
                weatherCode: model.weatherCode
                condition: model.condition
                highTemperature: model.highTemperature
                lowTemperature: model.lowTemperature
                precipitation: model.precipitation
                today: index === 0
            }
        }
    }

    Rectangle {
        anchors.left: dailyPanel.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 156
        width: 430
        height: 28
        radius: 14
        color: Weather.errorMessage.length > 0 ? "#3BEF6B69" : "#241B2735"
        visible: Weather.errorMessage.length > 0 || Weather.usingCache

        Text {
            anchors.centerIn: parent
            text: Weather.errorMessage.length > 0
                ? Weather.errorMessage
                : qsTr("网络不可用时已自动显示上次缓存数据")
            color: "#DFFFFFFF"
            font.pixelSize: 12
            elide: Text.ElideRight
            width: parent.width - 22
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Text {
        anchors.right: dailyPanel.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 160
        text: qsTr("天气数据由 Open-Meteo 提供")
        color: "#4FFFFFFF"
        font.pixelSize: 11
    }

    Rectangle {
        anchors.centerIn: parent
        width: 190
        height: 48
        radius: 24
        color: "#E21A2533"
        border.width: 1
        border.color: "#32FFFFFF"
        visible: Weather.loading
        z: 950

        Text {
            anchors.centerIn: parent
            text: qsTr("正在更新天气…")
            color: "#EFFFFFFF"
            font.pixelSize: 14
        }
    }

    PageChrome {
        anchors.fill: parent
    }
}
