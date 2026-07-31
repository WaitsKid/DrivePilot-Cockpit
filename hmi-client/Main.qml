import QtQuick
import QtQuick.Window
import QtWebEngine
import QtWebChannel
Window {
    id: mainWindow

    // 原始中控界面的设计分辨率。所有旧页面仍按 1521 × 856 编写，
    // 这里只统一缩放整张设计画布，不改动各页面内部的固定坐标。
    readonly property real designWidth: 1521
    readonly property real designHeight: 856
    readonly property bool portraitOrientation: Ui.screenRotation === 90
    readonly property real transformedDesignWidth: portraitOrientation ? designHeight : designWidth
    readonly property real transformedDesignHeight: portraitOrientation ? designWidth : designHeight
    readonly property real uiScale: Math.max(0.01,
                                             Math.min(width / transformedDesignWidth,
                                                      height / transformedDesignHeight))
    readonly property real scaledCanvasWidth: transformedDesignWidth * uiScale
    readonly property real scaledCanvasHeight: transformedDesignHeight * uiScale

    property real savedLandscapeWidth: 1521
    property real savedLandscapeHeight: 856
    property real savedLandscapeX: 0
    property real savedLandscapeY: 0
    readonly property real horizontalLetterbox: Math.max(0, (width - scaledCanvasWidth) / 2)
    readonly property real verticalLetterbox: Math.max(0, (height - scaledCanvasHeight) / 2)

    width: 1521
    height: 856
    minimumWidth: portraitOrientation ? 420 : 1100
    minimumHeight: portraitOrientation ? 620 : 620
    visible: true
    x: Screen.desktopAvailableWidth / 2 - width / 2
    y: Screen.desktopAvailableHeight / 2 - height / 2
    title: qsTr("DrivePilot Cockpit")
    color: "#090E16"
    flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowMinimizeButtonHint

    property int pageIndex: Ui.pageIndex
    readonly property bool mapPageActive: pageIndex === Ui.PAGE_MAP

    function syncNavigationEndpoints() {
        navigationBridge.endpointStateChanged(MapController.hasStart,
                                              MapController.startName,
                                              MapController.startLongitude,
                                              MapController.startLatitude,
                                              MapController.hasEnd,
                                              MapController.endName,
                                              MapController.endLongitude,
                                              MapController.endLatitude)
    }

    function syncNavigationVehicle() {
        navigationBridge.vehicleStateChanged(MapController.vehicleLongitude,
                                             MapController.vehicleLatitude,
                                             MapController.vehicleHeading,
                                             MapController.simulatedSpeed,
                                             MapController.navigating)
    }

    function centerWindowOnAvailableScreen() {
        x = Math.round((Screen.desktopAvailableWidth - width) / 2)
        y = Math.round((Screen.desktopAvailableHeight - height) / 2)
    }

    function applyOrientationWindowGeometry() {
        if (visibility !== Window.Windowed)
            return

        const availableWidth = Screen.desktopAvailableWidth
        const availableHeight = Screen.desktopAvailableHeight

        if (portraitOrientation) {
            savedLandscapeWidth = width
            savedLandscapeHeight = height
            savedLandscapeX = x
            savedLandscapeY = y

            const targetScale = Math.min(
                                      1.0,
                                      availableWidth * 0.92 / designHeight,
                                      availableHeight * 0.92 / designWidth)
            width = Math.round(designHeight * targetScale)
            height = Math.round(designWidth * targetScale)
            centerWindowOnAvailableScreen()
        } else {
            width = Math.min(savedLandscapeWidth, availableWidth * 0.96)
            height = Math.min(savedLandscapeHeight, availableHeight * 0.96)
            x = Math.max(0, Math.min(savedLandscapeX, availableWidth - width))
            y = Math.max(0, Math.min(savedLandscapeY, availableHeight - height))
        }
    }

    function loadPage(index) {
        switch (index) {
        case Ui.PAGE_HOME:
            pageLoader.source = "HMI/Home.qml"
            break
        case Ui.PAGE_APP:
            pageLoader.source = "HMI/App.qml"
            break
        case Ui.PAGE_AC:
            pageLoader.source = "HMI/AC.qml"
            break
        case Ui.PAGE_SETTINGS:
            pageLoader.source = "HMI/Settings.qml"
            break
        case Ui.PAGE_VEHICLE:
            pageLoader.source = "HMI/VehicleHealth.qml"
            break
        case Ui.PAGE_MUSIC:
            pageLoader.source = "HMI/MusicPage.qml"
            break
        case Ui.PAGE_WEATHER:
            pageLoader.source = "HMI/WeatherPage.qml"
            break
        case Ui.PAGE_ASSISTANT:
            pageLoader.source = "HMI/VoiceAssistantPage.qml"
            break
        case Ui.PAGE_CONTACTS:
            pageLoader.source = "HMI/ContactPage.qml"
            break
        case Ui.PAGE_VIDEO:
            pageLoader.source = "HMI/VideoCenterPage.qml"
            break
        case Ui.PAGE_CALCULATOR:
            pageLoader.source = "HMI/ScientificCalculatorPage.qml"
            break
        case Ui.PAGE_VECTOR_STUDIO:
            pageLoader.source = "HMI/VectorStudioPage.qml"
            break
        case Ui.PAGE_MAP:
            pageLoader.source = ""
            break
        default:
            pageLoader.source = "HMI/Home.qml"
            break
        }
    }

    Component.onCompleted: {
        savedLandscapeWidth = width
        savedLandscapeHeight = height
        savedLandscapeX = x
        savedLandscapeY = y
        MusicPlayer.volume = Ui.controlCenterMediaVolume
        VideoCenter.volume = Ui.controlCenterMediaVolume
        loadPage(Ui.pageIndex)
        DmsSystem.monitoringEnabled = Ui.settingsFatigueReminder
    }

    Connections {
        target: Ui

        function onToastRequested(message) {
            toastMessage.show(message)
        }

        function onControlCenterMediaVolumeChanged() {
            if (MusicPlayer.volume !== Ui.controlCenterMediaVolume)
                MusicPlayer.volume = Ui.controlCenterMediaVolume
            if (VideoCenter.volume !== Ui.controlCenterMediaVolume)
                VideoCenter.volume = Ui.controlCenterMediaVolume
        }

        function onScreenRotationChanged() {
            controlCenterPage.hide()
            mapControlCenterPage.hide()
            Qt.callLater(mainWindow.applyOrientationWindowGeometry)
        }

        function onSettingsFatigueReminderChanged() {
            if (!Ui.settingsFatigueReminder)
                dmsAlertToast.close()
            DmsSystem.monitoringEnabled = Ui.settingsFatigueReminder
        }
    }

    Connections {
        target: DmsSystem

        function onAlertRequested(message, fatigueLevel, eventId) {
            if (Ui.settingsFatigueReminder && DmsSystem.monitoringEnabled)
                dmsAlertToast.showAlert(message, fatigueLevel)
        }
    }

    Connections {
        target: MapController

        function onEndpointsChanged() {
            mainWindow.syncNavigationEndpoints()
        }

        function onVehiclePositionChanged() {
            mainWindow.syncNavigationVehicle()
        }
    }

    Connections {
        target: MusicPlayer

        function onVolumeChanged() {
            if (Ui.controlCenterMediaVolume !== MusicPlayer.volume)
                Ui.controlCenterMediaVolume = MusicPlayer.volume
        }

        function onPlayingChanged() {
            if (MusicPlayer.playing && VideoCenter.playing)
                VideoCenter.pause()
        }

        function onPlaybackError(message) {
            toastMessage.show(qsTr("音乐播放失败：%1").arg(message))
        }
    }

    Connections {
        target: VideoCenter

        function onVolumeChanged() {
            if (Ui.controlCenterMediaVolume !== VideoCenter.volume)
                Ui.controlCenterMediaVolume = VideoCenter.volume
        }

        function onPlayingChanged() {
            if (VideoCenter.playing && MusicPlayer.playing)
                MusicPlayer.pause()
        }

        function onPlaybackError(message) {
            toastMessage.show(qsTr("视频播放失败：%1").arg(message))
        }
    }

    // 窗口比例与 1521:856 不一致时显示的留边。
    // 不拉伸页面，也不改变原有图片比例。
    Rectangle {
        anchors.fill: parent
        color: mainWindow.color
        z: -10
    }

    WebEngineProfile {
        id: navigationProfile
        storageName: "BYDNavigation"
        offTheRecord: false
        httpCacheType: WebEngineProfile.DiskHttpCache
        httpCacheMaximumSize: 268435456
    }

    QtObject {
        id: navigationBridge
        WebChannel.id: "navigationBridge"

        readonly property string amapKey: MapController.amapJsKey
        readonly property string amapSecurityCode: MapController.amapSecurityCode
        readonly property string defaultCity: MapController.defaultCity
        readonly property double defaultLongitude: MapController.defaultLongitude
        readonly property double defaultLatitude: MapController.defaultLatitude

        signal searchRequested(int target, string keyword, string city)
        signal routeRequested(double startLongitude,
                              double startLatitude,
                              double endLongitude,
                              double endLatitude)
        signal endpointStateChanged(bool hasStart,
                                    string startName,
                                    double startLongitude,
                                    double startLatitude,
                                    bool hasEnd,
                                    string endName,
                                    double endLongitude,
                                    double endLatitude)
        signal vehicleStateChanged(double longitude,
                                   double latitude,
                                   double heading,
                                   double speed,
                                   bool navigating)
        signal commandRequested(string command, string payload)

        function reportMapReady() {
            MapController.reportMapReady()
            mainWindow.syncNavigationEndpoints()
            mainWindow.syncNavigationVehicle()
        }

        function reportMapError(message) {
            MapController.reportMapError(String(message || ""))
        }

        function receiveSearchResults(target, json) {
            MapController.receiveSearchResults(target, String(json || "[]"))
        }

        function receiveSearchError(target, message) {
            MapController.receiveSearchError(target, String(message || "地点搜索失败"))
        }

        function receiveRouteResult(json) {
            MapController.receiveRouteResult(String(json || "{}"))
        }

        function receiveRouteError(message) {
            MapController.receiveRouteError(String(message || "路线规划失败"))
        }

        function requestInitialState() {
            mainWindow.syncNavigationEndpoints()
            mainWindow.syncNavigationVehicle()
        }

        function reloadMapPage() {
            navigationWebView.reloadAndBypassCache()
        }
    }

    WebChannel {
        id: navigationChannel
        propertyUpdateInterval: 8
        registeredObjects: [navigationBridge]
    }

    WebEngineView {
        id: navigationWebView
        visible: mainWindow.mapPageActive && !mainWindow.portraitOrientation
        x: mainWindow.horizontalLetterbox + 108 * mainWindow.uiScale
        y: mainWindow.verticalLetterbox
        width: Math.min(mainWindow.width - x, 1414 * mainWindow.uiScale)
        height: Math.min(mainWindow.height - y, 856 * mainWindow.uiScale)
        z: 10
        url: Qt.resolvedUrl("Web/navigation.html")
        webChannel: navigationChannel
        profile: navigationProfile
        focus: false
        backgroundColor: "#DDEBF2"

        settings.javascriptEnabled: true
        settings.localStorageEnabled: true
        settings.localContentCanAccessFileUrls: true
        settings.localContentCanAccessRemoteUrls: true
        settings.accelerated2dCanvasEnabled: true
        settings.webGLEnabled: true
        settings.showScrollBars: false
        settings.focusOnNavigationEnabled: false

        // 地图接收鼠标拖动和滚轮，但不抢走 QML 的键盘焦点，
        // 否则用户拖动地图后 W/S 会被网页接收。
        Component.onCompleted: setActiveFocusOnPress(false)

        onVisibleChanged: {
            if (visible) {
                mainWindow.syncNavigationEndpoints()
                mainWindow.syncNavigationVehicle()
            }
        }

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.status === WebEngineView.LoadFailedStatus)
                MapController.reportMapError(qsTr("地图页面载入失败：%1").arg(loadRequest.errorString))
        }
    }

    // WebEngineView 在复杂 QML 叠层下的原生鼠标事件可能被上层透明 Item 截断。
    // 该层位于地图之上、所有导航控件之下，专门把拖动/滚轮按帧转成高德地图命令。
    // 因此地图拖动不再依赖 WebEngine 的事件穿透，也不会出现“松手后才移动”。
    Item {
        id: navigationMapInputLayer
        visible: mainWindow.mapPageActive && !mainWindow.portraitOrientation
        enabled: visible
        x: navigationWebView.x
        y: navigationWebView.y
        width: navigationWebView.width
        height: navigationWebView.height
        z: 15

        property real lastPointerX: 0
        property real lastPointerY: 0

        MouseArea {
            id: navigationMapMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            hoverEnabled: false
            preventStealing: true
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            onPressed: function(mouse) {
                navigationMapInputLayer.lastPointerX = mouse.x
                navigationMapInputLayer.lastPointerY = mouse.y
                navigationBridge.commandRequested("gesture-start", "")
                if (navigationOverlayLoader.item)
                    navigationOverlayLoader.item.forceActiveFocus()
                mouse.accepted = true
            }

            onPositionChanged: function(mouse) {
                if (!pressed)
                    return
                const deltaX = mouse.x - navigationMapInputLayer.lastPointerX
                const deltaY = mouse.y - navigationMapInputLayer.lastPointerY
                navigationMapInputLayer.lastPointerX = mouse.x
                navigationMapInputLayer.lastPointerY = mouse.y
                if (Math.abs(deltaX) < 0.01 && Math.abs(deltaY) < 0.01)
                    return
                navigationBridge.commandRequested(
                            "pan-by",
                            JSON.stringify({ deltaX: deltaX, deltaY: deltaY }))
                mouse.accepted = true
            }

            onReleased: function(mouse) {
                navigationBridge.commandRequested("gesture-end", "")
                if (navigationOverlayLoader.item)
                    navigationOverlayLoader.item.forceActiveFocus()
                mouse.accepted = true
            }

            onCanceled: navigationBridge.commandRequested("gesture-end", "")

            onDoubleClicked: function(mouse) {
                navigationBridge.commandRequested(
                            "zoom-at",
                            JSON.stringify({ direction: 1, x: mouse.x, y: mouse.y }))
                mouse.accepted = true
            }

            onWheel: function(wheel) {
                const direction = wheel.angleDelta.y >= 0 ? 1 : -1
                navigationBridge.commandRequested(
                            "zoom-at",
                            JSON.stringify({ direction: direction,
                                             x: wheel.x,
                                             y: wheel.y }))
                wheel.accepted = true
            }
        }
    }

    Item {
        id: navigationOverlayViewport
        anchors.fill: parent
        visible: mainWindow.mapPageActive && !mainWindow.portraitOrientation
        z: 20

        Item {
            id: navigationOverlayCanvas
            width: mainWindow.designWidth
            height: mainWindow.designHeight
            x: mainWindow.horizontalLetterbox
            y: mainWindow.verticalLetterbox
            transformOrigin: Item.TopLeft
            scale: mainWindow.uiScale

            Loader {
                id: navigationOverlayLoader
                anchors.fill: parent
                source: "HMI/MapPage.qml"

                onLoaded: {
                    if (item)
                        item.navigationBridge = navigationBridge
                }
            }

            ControlCenter {
                id: mapControlCenterPage
                width: 1414
                height: 856
                x: 108
                y: -height
                opacity: 0
                z: 600
            }

            Rectangle {
                width: 1292
                height: 60
                anchors.right: parent.right
                anchors.top: parent.top
                color: "transparent"
                z: 601

                SwipeArea {
                    anchors.fill: parent
                    onSwipeDown: mapControlCenterPage.show()
                }
            }

            Navigation {
                width: 108
                height: parent.height
                anchors.left: parent.left
                anchors.top: parent.top
                z: 700
                onShutdown: shutdownDialog.open()
            }
        }
    }

    Item {
        id: adaptiveViewport
        anchors.fill: parent
        clip: true
        visible: !mainWindow.mapPageActive || mainWindow.portraitOrientation
        enabled: visible
        z: 2

        Item {
            id: designCanvas
            width: mainWindow.designWidth
            height: mainWindow.designHeight
            anchors.centerIn: parent
            transformOrigin: Item.Center
            scale: mainWindow.uiScale
            rotation: Ui.screenRotation

            Behavior on rotation {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.InOutCubic
                }
            }

            Image {
                anchors.fill: parent
                visible: !mainWindow.mapPageActive
                source: "qrc:/Images/Home/base.png"
                fillMode: Image.Stretch
            }

            Loader {
                id: pageLoader
                anchors.fill: parent
                asynchronous: true

                onStatusChanged: {
                    if (status === Loader.Error)
                        Ui.showToast(qsTr("页面加载失败，请查看 Application Output 中的首条 QML 错误"))
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: pageLoader.status === Loader.Loading
                color: "#090E16"
                z: 50

                Text {
                    anchors.centerIn: parent
                    text: qsTr("正在载入页面…")
                    color: "#AFFFFFFF"
                    font.pixelSize: 20
                }
            }

            ControlCenter {
                id: controlCenterPage
                width: 1414
                height: 856
                x: 108
                y: -height
                opacity: 0
                z: 100
            }

            Rectangle {
                width: 1292
                height: 60
                anchors.right: parent.right
                anchors.top: parent.top
                color: "transparent"
                z: 101

                SwipeArea {
                    anchors.fill: parent
                    onSwipeDown: controlCenterPage.show()
                }
            }

            Navigation {
                width: 108
                height: parent.height
                anchors.left: parent.left
                anchors.top: parent.top
                z: 200
                onShutdown: shutdownDialog.open()
            }
        }
    }

    // Agent 工具桥保持常驻，确保跨页面工具调用能够连续执行。
    AiAgentToolBridge {
        id: aiAgentToolBridge
    }

    // 疲劳监测作为全局后台状态存在，不进入任何页面 Loader。
    // 因此切换页面或进入地图时，状态图标和告警仍然保持工作。
    DmsStatusIndicator {
        id: dmsStatusIndicator
        uiScale: mainWindow.uiScale
        x: mainWindow.portraitOrientation
           ? mainWindow.width - width - 18 * mainWindow.uiScale
           : mainWindow.horizontalLetterbox + 1234 * mainWindow.uiScale
        y: mainWindow.verticalLetterbox + 4 * mainWindow.uiScale
        z: 1400
        visible: Ui.settingsFatigueReminder && DmsSystem.monitoringEnabled
    }

    DmsAlertToast {
        id: dmsAlertToast
        uiScale: mainWindow.uiScale
        panelRightMargin: mainWindow.horizontalLetterbox + 24 * mainWindow.uiScale
        panelTopMargin: mainWindow.verticalLetterbox + 58 * mainWindow.uiScale
    }

    // Popup 不放进缩放画布，避免 Overlay 脱离 Item 变换后坐标错位。
    // 它根据 uiScale 调整尺寸，并按照设计画布底部计算位置。
    ToastMessage {
        id: toastMessage
        uiScale: mainWindow.uiScale
        toastBottomInset: mainWindow.verticalLetterbox + 200 * mainWindow.uiScale
    }

    ShutdownDialog {
        id: shutdownDialog
        uiScale: mainWindow.uiScale
        onConfirmed: Ui.quitApplication()
    }

    onPageIndexChanged: {
        mapControlCenterPage.hide()
        loadPage(pageIndex)
    }

    // === 窗口边缘拉伸手柄（保留用户原有实现，不跟随设计画布缩放）===
    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 4
        z: 1000
        cursorShape: Qt.SizeVerCursor
        onPressed: mainWindow.startSystemResize(Qt.TopEdge)
    }

    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 4
        z: 1000
        cursorShape: Qt.SizeVerCursor
        onPressed: mainWindow.startSystemResize(Qt.BottomEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        z: 1000
        cursorShape: Qt.SizeHorCursor
        onPressed: mainWindow.startSystemResize(Qt.LeftEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        z: 1000
        cursorShape: Qt.SizeHorCursor
        onPressed: mainWindow.startSystemResize(Qt.RightEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        width: 8
        height: 8
        z: 1001
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        width: 8
        height: 8
        z: 1001
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 8
        height: 8
        z: 1001
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 8
        height: 8
        z: 1001
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
}
