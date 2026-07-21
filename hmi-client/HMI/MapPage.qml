import QtQuick
import QtQuick.Controls
import BYD

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0
    focus: true

    property int activeSearchTarget: 0
    property int suggestionIndex: -1
    property bool forwardPressed: false
    property bool reversePressed: false
    property bool trafficEnabled: true
    property double lastDriveTick: 0
    property var navigationBridge: null

    function searchText(target) {
        return target === 1 ? startField.text : endField.text
    }

    function focusMapPoint(longitude, latitude, zoomLevel) {
        if (!navigationBridge)
            return
        navigationBridge.commandRequested("focus-point",
                                          JSON.stringify({
                                              longitude: longitude,
                                              latitude: latitude,
                                              zoom: zoomLevel || 16
                                          }))
    }

    function triggerSearch(target, text) {
        if (target !== 1 && target !== 2)
            return
        const keyword = String(text || "").trim()
        if (keyword.length === 0) {
            if (target === 1)
                MapController.clearStartSuggestions()
            else
                MapController.clearEndSuggestions()
            return
        }
        if (!navigationBridge)
            return
        MapController.beginSearch(target, keyword)
        navigationBridge.searchRequested(target, keyword, MapController.defaultCity)
    }

    function chooseSuggestion(index) {
        if (index < 0)
            return
        const selectedTarget = activeSearchTarget
        if (selectedTarget === 1) {
            MapController.selectStartSuggestion(index)
        } else if (selectedTarget === 2) {
            MapController.selectEndSuggestion(index)
        }
        searchDebounce.stop()
        activeSearchTarget = 0
        suggestionIndex = -1
        // endpointStateChanged 与 focus-point 通过同一条 WebChannel 发送，下一事件循环再聚焦，
        // 避免标记更新覆盖刚开始的地图跳转。
        Qt.callLater(function() {
            if (selectedTarget === 1)
                root.focusMapPoint(MapController.startLongitude, MapController.startLatitude, 16)
            else if (selectedTarget === 2)
                root.focusMapPoint(MapController.endLongitude, MapController.endLatitude, 16)
        })
        root.forceActiveFocus()
    }

    function requestRoute() {
        if (!navigationBridge || !MapController.beginRoutePlanning())
            return
        navigationBridge.routeRequested(MapController.startLongitude,
                                        MapController.startLatitude,
                                        MapController.endLongitude,
                                        MapController.endLatitude)
        root.forceActiveFocus()
    }

    function syncEndpointsToWeb() {
        if (!navigationBridge)
            return
        navigationBridge.endpointStateChanged(MapController.hasStart,
                                              MapController.startName,
                                              MapController.startLongitude,
                                              MapController.startLatitude,
                                              MapController.hasEnd,
                                              MapController.endName,
                                              MapController.endLongitude,
                                              MapController.endLatitude)
    }

    function syncVehicleToWeb() {
        if (!navigationBridge)
            return
        navigationBridge.vehicleStateChanged(MapController.vehicleLongitude,
                                             MapController.vehicleLatitude,
                                             MapController.vehicleHeading,
                                             MapController.simulatedSpeed,
                                             MapController.navigating)
    }

    function stopDriveKeys() {
        forwardPressed = false
        reversePressed = false
        MapController.stopVehicleMotion()
    }

    Keys.onPressed: function(event) {
        if (!MapController.navigating || event.isAutoRepeat)
            return
        if (event.key === Qt.Key_W) {
            forwardPressed = true
            reversePressed = false
            // W/S 只控制车辆沿路线前后移动，不改变用户当前缩放和地图中心。
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            reversePressed = true
            forwardPressed = false
            event.accepted = true
        }
    }

    Keys.onReleased: function(event) {
        if (!MapController.navigating || event.isAutoRepeat)
            return
        if (event.key === Qt.Key_W) {
            forwardPressed = false
            MapController.stopVehicleMotion()
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            reversePressed = false
            MapController.stopVehicleMotion()
            event.accepted = true
        }
    }

    onActiveFocusChanged: {
        if (!activeFocus && MapController.navigating)
            stopDriveKeys()
    }

    Timer {
        id: driveTimer
        // 40 FPS 足够保证箭头连续，同时显著减少 QWebChannel 与 WebEngine 主线程压力。
        interval: 25
        repeat: true
        running: MapController.navigating && (root.forwardPressed || root.reversePressed)

        onRunningChanged: {
            root.lastDriveTick = Date.now()
            if (!running)
                MapController.stopVehicleMotion()
        }

        onTriggered: {
            const now = Date.now()
            const elapsedSeconds = Math.max(0.001,
                                            Math.min(0.05, (now - root.lastDriveTick) / 1000.0))
            root.lastDriveTick = now
            const metersPerSecond = MapController.simulationSpeedKmh / 3.6
            const direction = root.forwardPressed ? 1.0 : -1.0
            // simulateDrive() 会触发 vehiclePositionChanged，底部 Connections 已负责
            // 同步到 WebEngine。这里不再重复发送，避免每帧两次 QWebChannel 调用。
            MapController.simulateDrive(direction * metersPerSecond * elapsedSeconds)
        }
    }

    Timer {
        id: searchDebounce
        interval: 300
        repeat: false
        onTriggered: root.triggerSearch(root.activeSearchTarget,
                                        root.searchText(root.activeSearchTarget))
    }

    Rectangle {
        anchors.fill: parent
        color: "#26071320"
        visible: !MapController.mapReady
        z: 50
    }

    Rectangle {
        id: searchPanel
        visible: !MapController.navigating
        width: 420
        height: 198
        anchors.left: parent.left
        anchors.leftMargin: 28
        anchors.top: parent.top
        anchors.topMargin: 34
        radius: 25
        color: "#F20D1A27"
        border.width: 1
        border.color: "#3D89BFEA"
        z: 200

        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            Row {
                width: parent.width
                height: 68
                spacing: 10

                Rectangle {
                    width: 38
                    height: 38
                    radius: 19
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#244C779D"
                    border.width: 1
                    border.color: "#508FC8EE"
                    Label { anchors.centerIn: parent; text: "起"; color: "#74D9FF"; font.pixelSize: 15; font.bold: true }
                }

                TextField {
                    id: startField
                    width: parent.width - 96
                    height: 56
                    anchors.verticalCenter: parent.verticalCenter
                    placeholderText: qsTr("起点，默认使用当前位置")
                    text: MapController.hasStart ? MapController.startName : ""
                    color: "#FFFFFF"
                    placeholderTextColor: "#758A9C"
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
                    leftPadding: 16
                    rightPadding: 44
                    selectByMouse: true
                    background: Rectangle {
                        radius: 16
                        color: startField.activeFocus ? "#253B50" : "#172838"
                        border.width: 1
                        border.color: startField.activeFocus ? "#5DBFEF" : "#263E53"
                    }

                    onActiveFocusChanged: {
                        if (activeFocus) {
                            root.activeSearchTarget = 1
                            root.suggestionIndex = -1
                            if (text.length > 0)
                                searchDebounce.restart()
                        }
                    }
                    onTextEdited: {
                        root.activeSearchTarget = 1
                        root.suggestionIndex = -1
                        searchDebounce.restart()
                    }
                    Keys.onDownPressed: {
                        root.suggestionIndex = Math.min(root.suggestionIndex + 1,
                                                        suggestionList.count - 1)
                    }
                    Keys.onUpPressed: root.suggestionIndex = Math.max(-1, root.suggestionIndex - 1)
                    Keys.onReturnPressed: {
                        if (root.suggestionIndex >= 0)
                            root.chooseSuggestion(root.suggestionIndex)
                        else
                            root.triggerSearch(1, text)
                    }

                    ToolButton {

                        hoverEnabled: false
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 38
                        height: 38
                        text: "⌖"
                        onClicked: {
                            MapController.useCurrentLocationAsStart()
                            startField.text = MapController.startName
                            root.focusMapPoint(MapController.startLongitude,
                                               MapController.startLatitude,
                                               16)
                            root.activeSearchTarget = 0
                            root.forceActiveFocus()
                        }
                        background: Rectangle { radius: 12; color: "transparent" }
                        contentItem: Label { text: parent.text; color: "#6DDCFF"; font.pixelSize: 19; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }

            Rectangle { width: parent.width - 48; height: 1; anchors.horizontalCenter: parent.horizontalCenter; color: "#243D51" }

            Row {
                width: parent.width
                height: 68
                spacing: 10

                Rectangle {
                    width: 38
                    height: 38
                    radius: 19
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#46313B"
                    border.width: 1
                    border.color: "#B6566F"
                    Label { anchors.centerIn: parent; text: "终"; color: "#FF8DA2"; font.pixelSize: 15; font.bold: true }
                }

                TextField {
                    id: endField
                    width: parent.width - 96
                    height: 56
                    anchors.verticalCenter: parent.verticalCenter
                    placeholderText: qsTr("输入目的地")
                    text: MapController.hasEnd ? MapController.endName : ""
                    color: "#FFFFFF"
                    placeholderTextColor: "#758A9C"
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
                    leftPadding: 16
                    rightPadding: 44
                    selectByMouse: true
                    background: Rectangle {
                        radius: 16
                        color: endField.activeFocus ? "#253B50" : "#172838"
                        border.width: 1
                        border.color: endField.activeFocus ? "#5DBFEF" : "#263E53"
                    }

                    onActiveFocusChanged: {
                        if (activeFocus) {
                            root.activeSearchTarget = 2
                            root.suggestionIndex = -1
                            if (text.length > 0)
                                searchDebounce.restart()
                        }
                    }
                    onTextEdited: {
                        root.activeSearchTarget = 2
                        root.suggestionIndex = -1
                        searchDebounce.restart()
                    }
                    Keys.onDownPressed: {
                        root.suggestionIndex = Math.min(root.suggestionIndex + 1,
                                                        suggestionList.count - 1)
                    }
                    Keys.onUpPressed: root.suggestionIndex = Math.max(-1, root.suggestionIndex - 1)
                    Keys.onReturnPressed: {
                        if (root.suggestionIndex >= 0)
                            root.chooseSuggestion(root.suggestionIndex)
                        else
                            root.triggerSearch(2, text)
                    }

                    ToolButton {

                        hoverEnabled: false
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 38
                        height: 38
                        text: endField.text.length > 0 ? "×" : "⌕"
                        onClicked: {
                            if (endField.text.length > 0) {
                                endField.clear()
                                MapController.clearEnd()
                                MapController.clearEndSuggestions()
                            } else {
                                endField.forceActiveFocus()
                            }
                        }
                        background: Rectangle { radius: 12; color: "transparent" }
                        contentItem: Label { text: parent.text; color: "#9FCBE7"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }
        }
    }

    Rectangle {
        id: suggestionPanel
        visible: root.activeSearchTarget !== 0
                 && (suggestionList.count > 0
                     || MapController.startSearching
                     || MapController.endSearching)
        width: searchPanel.width
        height: Math.min(360, 30 + suggestionList.contentHeight)
        anchors.left: searchPanel.left
        anchors.top: searchPanel.bottom
        anchors.topMargin: 10
        radius: 22
        color: "#F20D1A27"
        border.width: 1
        border.color: "#3D89BFEA"
        clip: true
        z: 210

        BusyIndicator {
            anchors.centerIn: parent
            running: MapController.startSearching || MapController.endSearching
            visible: running && suggestionList.count === 0
        }

        ListView {
            id: suggestionList
            anchors.fill: parent
            anchors.margins: 12
            spacing: 4
            clip: true
            model: root.activeSearchTarget === 1
                   ? MapController.startSuggestions
                   : MapController.endSuggestions

            delegate: Item {
                width: suggestionList.width
                height: 68

                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    color: root.suggestionIndex === index
                           ? "#314B68" : "transparent"
                    border.width: root.suggestionIndex === index ? 1 : 0
                    border.color: "#628FC2"
                }

                Rectangle {
                    width: 34
                    height: 34
                    radius: 17
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#36E7EA" }
                        GradientStop { position: 1.0; color: "#287FEA" }
                    }
                    Label { anchors.centerIn: parent; text: "⌖"; color: "#FFFFFF"; font.pixelSize: 17 }
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 56
                    anchors.right: distanceLabel.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Label { width: parent.width; text: name; color: "#FFFFFF"; font.pixelSize: 15; font.bold: true; elide: Text.ElideRight }
                    Label { width: parent.width; text: address.length > 0 ? address : district; color: "#91ABC0"; font.pixelSize: 12; elide: Text.ElideRight }
                }

                Label {
                    id: distanceLabel
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: distanceText
                    color: "#70C9FF"
                    font.pixelSize: 12
                    font.bold: true
                }

                TapHandler { onTapped: root.chooseSuggestion(index) }
            }
        }
    }

    Column {
        id: mapButtons
        anchors.right: parent.right
        anchors.rightMargin: 28
        anchors.top: parent.top
        anchors.topMargin: 34
        spacing: 10
        z: 220

        Repeater {
            model: [
                { icon: "+", command: "zoom-in", tip: qsTr("放大") },
                { icon: "−", command: "zoom-out", tip: qsTr("缩小") },
                { icon: "⌖", command: "follow-vehicle", tip: qsTr("回到车辆") },
                { icon: "全", command: "fit-route", tip: qsTr("路线全览") }
            ]

            delegate: ToolButton {
                hoverEnabled: false
                required property var modelData
                width: 52
                height: 52
                text: modelData.icon
                ToolTip.visible: false
                ToolTip.text: modelData.tip
                onClicked: {
                    if (navigationBridge)
                        navigationBridge.commandRequested(modelData.command, "")
                    root.forceActiveFocus()
                }
                background: Rectangle {
                    radius: 17
                    color: parent.pressed ? "#355A76" : "#E7132433"
                    border.width: 1
                    border.color: "#3A7EA8C9"
                }
                contentItem: Label {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: parent.text === "全" ? 15 : 24
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        ToolButton {

            hoverEnabled: false
            width: 52
            height: 52
            text: root.trafficEnabled ? "路" : "净"
            ToolTip.visible: false
            ToolTip.text: root.trafficEnabled ? qsTr("关闭实时路况") : qsTr("打开实时路况")
            onClicked: {
                root.trafficEnabled = !root.trafficEnabled
                if (navigationBridge)
                    navigationBridge.commandRequested("traffic", root.trafficEnabled ? "1" : "0")
                root.forceActiveFocus()
            }
            background: Rectangle {
                radius: 17
                color: root.trafficEnabled ? "#245B777A" : "#E7132433"
                border.width: 1
                border.color: root.trafficEnabled ? "#52D9D5" : "#3A7EA8C9"
            }
            contentItem: Label { text: parent.text; color: root.trafficEnabled ? "#7FFFEF" : "#FFFFFF"; font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
    }

    Rectangle {
        id: navigationBanner
        visible: MapController.navigating
        width: 680
        // 指令文字允许完整换行，卡片高度跟随文字内容自动增长。
        height: Math.max(104, instructionColumn.implicitHeight + 36)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 30
        radius: 28
        color: "#F20D1A27"
        border.width: 1
        border.color: "#448FD8FF"
        z: 230

        Row {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 18

            Rectangle {
                width: 66
                height: 66
                anchors.verticalCenter: parent.verticalCenter
                radius: 21
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#36E7EA" }
                    GradientStop { position: 1.0; color: "#2078FF" }
                }
                Label { anchors.centerIn: parent; text: "↑"; color: "#FFFFFF"; font.pixelSize: 38; font.bold: true }
            }

            Column {
                id: instructionColumn
                width: 430
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7
                Label {
                    width: parent.width
                    text: MapController.currentInstruction.length > 0
                          ? MapController.currentInstruction
                          : qsTr("沿规划路线行驶")
                    color: "#FFFFFF"
                    font.pixelSize: 22
                    font.bold: true
                    wrapMode: Text.WordWrap
                    elide: Text.ElideNone
                    lineHeight: 1.12
                }
                Label {
                    width: parent.width
                    text: MapController.nextRoadName.length > 0
                          ? qsTr("下一道路：%1").arg(MapController.nextRoadName)
                          : qsTr("按住 W 前进，S 后退")
                    color: "#9FC4E1"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    elide: Text.ElideNone
                    lineHeight: 1.12
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Label { width: 100; text: MapController.remainingDistanceText; color: "#76E7FF"; font.pixelSize: 20; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Label { width: 100; text: qsTr("%1 到达").arg(MapController.arrivalTimeText); color: "#93AFC4"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
            }
        }
    }

    Rectangle {
        id: routeCard
        visible: MapController.routeReady
        width: 420
        height: MapController.navigating ? 235 : 190
        anchors.left: parent.left
        anchors.leftMargin: 28
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 34
        radius: 26
        color: "#F20D1A27"
        border.width: 1
        border.color: "#3D89BFEA"
        z: 220

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Row {
                width: parent.width
                spacing: 18
                Column {
                    width: 100
                    Label { text: MapController.navigating ? MapController.remainingDistanceText : MapController.routeDistanceText; color: "#FFFFFF"; font.pixelSize: 25; font.bold: true }
                    Label { text: MapController.navigating ? qsTr("剩余距离") : qsTr("路线距离"); color: "#809DB3"; font.pixelSize: 12 }
                }
                Rectangle { width: 1; height: 46; color: "#294457" }
                Column {
                    width: 120
                    Label { text: MapController.navigating ? MapController.remainingDurationText : MapController.routeDurationText; color: "#FFFFFF"; font.pixelSize: 20; font.bold: true }
                    Label { text: MapController.navigating ? qsTr("预计剩余") : qsTr("预计用时"); color: "#809DB3"; font.pixelSize: 12 }
                }
                Column {
                    width: 110
                    visible: MapController.navigating
                    Label { text: Math.abs(Math.round(MapController.simulatedSpeed)); color: "#77E7FF"; font.pixelSize: 25; font.bold: true; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                    Label { text: qsTr("km/h（模拟）"); color: "#809DB3"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                }
            }

            Label {
                width: parent.width
                text: MapController.startName + "  →  " + MapController.endName
                color: "#FFFFFF"
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
            }

            Row {
                visible: MapController.navigating
                width: parent.width
                height: 34
                spacing: 8

                Slider {
                    id: speedSlider
                    width: parent.width - speedValueLabel.width - 68 - parent.spacing * 3
                    height: 34
                    from: 60
                    to: 3600
                    stepSize: 60
                    value: MapController.simulationSpeedKmh
                    onMoved: MapController.setSimulationSpeedKmh(value)
                    background: Rectangle { x: speedSlider.leftPadding; y: speedSlider.topPadding + speedSlider.availableHeight / 2 - height / 2; width: speedSlider.availableWidth; height: 6; radius: 3; color: "#29465B"; Rectangle { width: speedSlider.visualPosition * parent.width; height: parent.height; radius: 3; color: "#48DCEA" } }
                    handle: Rectangle { x: speedSlider.leftPadding + speedSlider.visualPosition * (speedSlider.availableWidth - width); y: speedSlider.topPadding + speedSlider.availableHeight / 2 - height / 2; width: 20; height: 20; radius: 10; color: "#FFFFFF"; border.width: 3; border.color: "#48DCEA" }
                }

                Label {
                    id: speedValueLabel
                    width: 82
                    height: 34
                    text: qsTr("%1 km/min").arg((MapController.simulationSpeedKmh / 60).toFixed(0))
                    color: "#70E3F7"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                ToolButton {
                    hoverEnabled: false
                    width: 34
                    height: 34
                    text: "−"
                    onClicked: MapController.setSimulationSpeedKmh(MapController.simulationSpeedKmh - 300)
                    background: Rectangle { radius: 10; color: "#1B3143" }
                    contentItem: Label { text: parent.text; color: "#FFFFFF"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                ToolButton {
                    hoverEnabled: false
                    width: 34
                    height: 34
                    text: "+"
                    onClicked: MapController.setSimulationSpeedKmh(MapController.simulationSpeedKmh + 300)
                    background: Rectangle { radius: 10; color: "#1B3143" }
                    contentItem: Label { text: parent.text; color: "#FFFFFF"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }

            Row {
                width: parent.width
                spacing: 10

                Button {

                    hoverEnabled: false
                    width: 112
                    height: 46
                    text: qsTr("路线全览")
                    onClicked: {
                        if (navigationBridge)
                            navigationBridge.commandRequested("fit-route", "")
                    }
                    background: Rectangle { radius: 15; color: "#1B3042"; border.width: 1; border.color: "#355873" }
                    contentItem: Label { text: parent.text; color: "#B7D5E9"; font.pixelSize: 14; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {

                    hoverEnabled: false
                    width: parent.width - 122
                    height: 46
                    text: MapController.navigating ? qsTr("暂停导航") : qsTr("开始导航")
                    onClicked: {
                        if (MapController.navigating) {
                            MapController.stopNavigation()
                            if (navigationBridge)
                                navigationBridge.commandRequested("navigation-stop", "")
                        } else {
                            MapController.startNavigation()
                            if (navigationBridge)
                                navigationBridge.commandRequested("navigation-start", "")
                            root.forceActiveFocus()
                        }
                    }
                    background: Rectangle {
                        radius: 15
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: MapController.navigating ? "#E76C58" : "#36E7EA" }
                            GradientStop { position: 1.0; color: MapController.navigating ? "#C84251" : "#2078FF" }
                        }
                    }
                    contentItem: Label { text: parent.text; color: "#FFFFFF"; font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    Button {

        hoverEnabled: false
        visible: !MapController.routeReady && MapController.hasStart && MapController.hasEnd
        width: 190
        height: 58
        anchors.left: parent.left
        anchors.leftMargin: 28
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 34
        text: MapController.routeLoading ? qsTr("正在规划…") : qsTr("规划驾车路线")
        enabled: !MapController.routeLoading && MapController.mapReady
        z: 220
        onClicked: root.requestRoute()
        background: Rectangle {
            radius: 18
            opacity: parent.enabled ? 1.0 : 0.55
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#36E7EA" }
                GradientStop { position: 1.0; color: "#2078FF" }
            }
        }
        contentItem: Label { text: parent.text; color: "#FFFFFF"; font.pixelSize: 16; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    Rectangle {
        id: routeStepsPanel
        visible: MapController.routeReady && !MapController.navigating
        width: 360
        height: 260
        anchors.right: parent.right
        anchors.rightMargin: 96
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 34
        radius: 24
        color: "#F20D1A27"
        border.width: 1
        border.color: "#3D89BFEA"
        z: 220

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Row {
                width: parent.width
                Label { text: qsTr("路线指引"); color: "#FFFFFF"; font.pixelSize: 17; font.bold: true }
                Item { width: parent.width - 160; height: 1 }
                Label { text: qsTr("%1 步").arg(routeStepList.count); color: "#74D9FF"; font.pixelSize: 13 }
            }

            Rectangle { width: parent.width; height: 1; color: "#294457" }

            ListView {
                id: routeStepList
                width: parent.width
                height: parent.height - 48
                model: MapController.routeSteps
                spacing: 5
                clip: true

                delegate: Rectangle {
                    required property string instruction
                    required property string roadName
                    required property string distanceText
                    required property bool active
                    width: routeStepList.width
                    height: 56
                    radius: 13
                    color: active ? "#264A5E" : "#122535"
                    border.width: active ? 1 : 0
                    border.color: "#4DD9E7"

                    Rectangle {
                        width: 28; height: 28; radius: 14
                        anchors.left: parent.left; anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        color: active ? "#39DDE4" : "#29475B"
                        Label { anchors.centerIn: parent; text: index + 1; color: active ? "#09232C" : "#D6E7F2"; font.pixelSize: 12; font.bold: true }
                    }
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 48
                        anchors.right: distanceStepLabel.left; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3
                        Label { width: parent.width; text: instruction; color: "#FFFFFF"; font.pixelSize: 13; font.bold: true; elide: Text.ElideRight }
                        Label { width: parent.width; text: roadName.length > 0 ? roadName : qsTr("未命名道路"); color: "#849FB3"; font.pixelSize: 11; elide: Text.ElideRight }
                    }
                    Label {
                        id: distanceStepLabel
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: distanceText
                        color: "#70C9FF"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }
    }

    Rectangle {
        id: keyboardHint
        visible: MapController.navigating
        width: 350
        height: 62
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 34
        radius: 20
        color: "#E90D1A27"
        border.width: 1
        border.color: root.forwardPressed || root.reversePressed ? "#54DDE9" : "#355A72"
        z: 220

        Row {
            anchors.centerIn: parent
            spacing: 12
            Repeater {
                model: [
                    { key: "W", text: qsTr("前进"), active: root.forwardPressed },
                    { key: "S", text: qsTr("后退"), active: root.reversePressed }
                ]
                delegate: Row {
                    required property var modelData
                    spacing: 7
                    Rectangle {
                        width: 34; height: 34; radius: 10
                        color: modelData.active ? "#3CDDE2" : "#1A3042"
                        border.width: 1
                        border.color: modelData.active ? "#8AFFFF" : "#35546A"
                        Label { anchors.centerIn: parent; text: modelData.key; color: modelData.active ? "#08202B" : "#FFFFFF"; font.pixelSize: 15; font.bold: true }
                    }
                    Label { anchors.verticalCenter: parent.verticalCenter; text: modelData.text; color: "#AFC8D9"; font.pixelSize: 13 }
                }
            }
            Rectangle { width: 1; height: 30; color: "#355069" }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("推进 %1 km/min").arg((MapController.simulationSpeedKmh / 60).toFixed(0))
                color: "#70E3F7"
                font.pixelSize: 13
                font.bold: true
            }
        }
    }

    Rectangle {
        width: Math.min(580, statusLabel.implicitWidth + 58)
        height: 42
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: MapController.navigating ? 110 : 34
        radius: 18
        color: "#DF0B1824"
        border.width: 1
        border.color: MapController.lastError.length > 0 ? "#72E86570" : "#324F6578"
        z: 240

        Row {
            anchors.centerIn: parent
            spacing: 10
            Rectangle {
                width: 9; height: 9; radius: 5
                color: MapController.lastError.length > 0
                       ? "#EF6B73"
                       : MapController.mapReady ? "#43D68A" : "#F0C65D"
            }
            Label {
                id: statusLabel
                text: MapController.lastError.length > 0
                      ? MapController.lastError
                      : MapController.statusText
                color: MapController.lastError.length > 0 ? "#FF9FA7" : "#A9C5D9"
                font.pixelSize: 13
            }
        }
    }

    Rectangle {
        visible: !MapController.configured
        width: 530
        height: 210
        anchors.centerIn: parent
        radius: 28
        color: "#F40D1A27"
        border.width: 1
        border.color: "#60E08B55"
        z: 500

        Column {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 14
            Label { width: parent.width; text: qsTr("高德 JS API 尚未配置"); color: "#FFFFFF"; font.pixelSize: 23; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            Label {
                width: parent.width
                text: qsTr("请在运行目录 config.json 中填写 amap_js_key 和 amap_js_security_code。保存后点击重新读取。")
                color: "#AFC4D5"
                font.pixelSize: 15
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                hoverEnabled: false
                width: 160; height: 44; anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("重新读取配置")
                onClicked: {
                    MapController.reloadConfiguration()
                    if (navigationBridge)
                        navigationBridge.reloadMapPage()
                }
                background: Rectangle { radius: 14; color: "#2478AA" }
                contentItem: Label { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }

    Connections {
        target: MapController

        function onEndpointsChanged() {
            startField.text = MapController.hasStart ? MapController.startName : ""
            endField.text = MapController.hasEnd ? MapController.endName : ""
            root.syncEndpointsToWeb()
        }

        function onVehiclePositionChanged() {
            root.syncVehicleToWeb()
        }

        function onRouteStateChanged() {
            if (!MapController.routeReady
                    && !MapController.routeLoading
                    && navigationBridge) {
                navigationBridge.commandRequested("clear-route", "")
            }
        }

        function onNavigationStateChanged() {
            if (!MapController.navigating)
                root.stopDriveKeys()
        }
    }

    Component.onCompleted: {
        MapController.reloadConfiguration()
        MapController.clearRoute()
        if (MapController.hasStart)
            startField.text = MapController.startName
        if (MapController.hasEnd)
            endField.text = MapController.endName
        root.forceActiveFocus()
    }
}
