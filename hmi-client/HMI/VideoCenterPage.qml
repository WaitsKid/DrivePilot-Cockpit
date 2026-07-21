import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import BYD

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0

    FileDialog {
        id: videoImportDialog
        title: qsTr("导入视频到车机媒体库")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("视频文件 (*.mp4 *.mkv *.mov *.avi *.webm *.m4v *.wmv)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            var files = []
            for (var index = 0; index < selectedFiles.length; ++index)
                files.push(selectedFiles[index])
            VideoCenter.importFiles(files)
        }
    }

    property bool theaterMode: false
    property bool showTheaterControls: true

    function formatTime(milliseconds) {
        return VideoCenter.formatTime(milliseconds)
    }

    function setMediaVolume(value) {
        const boundedVolume = Math.max(0, Math.min(10, Math.round(value)))

        if (VideoCenter.volume !== boundedVolume)
            VideoCenter.volume = boundedVolume
        if (Ui.controlCenterMediaVolume !== boundedVolume)
            Ui.controlCenterMediaVolume = boundedVolume
    }

    function toggleMediaMute() {
        setMediaVolume((VideoCenter.muted || VideoCenter.volume === 0) ? 1 : 0)
    }

    function showControls() {
        if (!root.theaterMode)
            return
        root.showTheaterControls = true
        hideControlsTimer.restart()
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    PageChrome {
        anchors.fill: parent
        visible: !root.theaterMode
    }

    Label {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 54
        anchors.top: parent.top
        anchors.topMargin: 64
        text: qsTr("视频中心")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
        visible: !root.theaterMode
    }

    Rectangle {
        id: playerPanel
        x: root.theaterMode ? 0 : 48
        y: root.theaterMode ? 0 : 116
        width: root.theaterMode ? root.width-108 : 852
        height: root.theaterMode ? root.height : 580
        radius: root.theaterMode ? 0 : 24
        color: root.theaterMode ? "#000000" : "#B518202C"
        border.width: root.theaterMode ? 0 : 1
        border.color: "#24FFFFFF"
        z: root.theaterMode ? 500 : 1

        Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on radius { NumberAnimation { duration: 120 } }

        Rectangle {
            id: videoFrame
            x: root.theaterMode ? 0 : 18
            y: root.theaterMode ? 0 : 18
            width: root.theaterMode ? parent.width : parent.width - 36
            height: root.theaterMode ? parent.height : 404
            radius: root.theaterMode ? 0 : 18
            color: "#05080D"
            clip: true

            Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            Behavior on radius { NumberAnimation { duration: 120 } }

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                endOfStreamPolicy: VideoOutput.KeepLastFrame
                Component.onCompleted: VideoCenter.attachVideoOutput(videoOutput)
                Component.onDestruction: VideoCenter.detachVideoOutput(videoOutput)
            }

            Image {
                anchors.fill: parent
                source: VideoCenter.posterSource
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                visible: !VideoCenter.sourceReady || (!VideoCenter.playing && VideoCenter.position <= 0)
                opacity: 0.74
            }

            Rectangle {
                anchors.fill: parent
                color: "#42000000"
                visible: !VideoCenter.sourceReady || (!VideoCenter.playing && VideoCenter.position <= 0)
            }

            Rectangle {
                width: 92
                height: 92
                anchors.centerIn: parent
                radius: 46
                color: playOverlayButton.pressed ? "#D95277A8" : "#BF25354A"
                border.width: 1
                border.color: "#62FFFFFF"
                visible: !VideoCenter.playing && !VideoCenter.loading && VideoCenter.currentAvailable && !root.theaterMode
                z: 2

                Label {
                    anchors.centerIn: parent
                    text: "▶"
                    color: "#FFFFFF"
                    font.pixelSize: 36
                    leftPadding: 5
                }

                MouseArea {
                    id: playOverlayButton
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: VideoCenter.play()
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 12
                visible: VideoCenter.loading
                z: 2

                Rectangle {
                    id: loadingRing
                    width: 52
                    height: 52
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 26
                    color: "transparent"
                    border.width: 4
                    border.color: "#70A7F5"

                    Rectangle {
                        width: 14
                        height: 14
                        radius: 7
                        color: "#FFFFFF"
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    RotationAnimation on rotation {
                        running: loadingRing.visible
                        loops: Animation.Infinite
                        from: 0
                        to: 360
                        duration: 820
                    }
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("正在加载视频")
                    color: "#EFFFFFFF"
                    font.pixelSize: 15
                }
            }

            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 80, 560)
                spacing: 12
                visible: !VideoCenter.currentAvailable
                z: 2

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "!"
                    color: "#FFB4A8"
                    font.pixelSize: 52
                    font.weight: Font.Bold
                }

                Label {
                    width: parent.width
                    text: qsTr("未找到视频文件")
                    color: "#FFFFFF"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    width: parent.width
                    text: qsTr("请确认 Videos 目录已复制到可执行文件旁边")
                    color: "#AFFFFFFF"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // 沉浸模式手势层
            SwipeArea {
                id: videoGestureArea
                anchors.fill: parent
                z: 1
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                visible: root.theaterMode

                onPositionChanged: root.showControls()
                onClicked: {
                    if (root.showTheaterControls) {
                        VideoCenter.playPause()
                    }
                    root.showControls()
                }
                onDoubleClicked: root.theaterMode = !root.theaterMode
                onSwipe: function(direction) {
                    if (direction === "left")
                        VideoCenter.seek(Math.max(0, VideoCenter.position - 5000))
                    else if (direction === "right")
                        VideoCenter.seek(Math.min(VideoCenter.duration, VideoCenter.position + 5000))
                }
            }

            // 普通模式点击层（与沉浸模式分开，避免手势冲突）
            MouseArea {
                anchors.fill: parent
                z: 1
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                visible: !root.theaterMode
                onDoubleClicked: root.theaterMode = !root.theaterMode
                onClicked: VideoCenter.playPause()
            }

            // 沉浸模式浮动控制栏
            Rectangle {
                id: theaterControlsOverlay
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 96
                color: "#A8070A0F"
                z: 3
                opacity: root.theaterMode && root.showTheaterControls ? 1 : 0
                visible: opacity > 0

                Behavior on opacity { NumberAnimation { duration: 220 } }

                ColorSlider {
                    id: theaterProgress
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    anchors.top: parent.top
                    anchors.topMargin: 12
                    height: 19
                    minValue: 0
                    maxValue: Math.max(1, VideoCenter.duration)
                    value: VideoCenter.position
                    startColor: "#70A7F5"
                    endColor: "#52E6FB"
                    cornerRadius: 10

                    onValueModified: VideoCenter.seek(value)
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 14
                    spacing: 16

                    Button {
                        width: 44
                        height: 36
                        hoverEnabled: false
                        contentItem: Label {
                            text: VideoCenter.playing ? "Ⅱ" : "▶"
                            color: "#FFFFFF"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 12; color: parent.down ? "#35FFFFFF" : "transparent" }
                        onClicked: VideoCenter.playPause()
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.formatTime(VideoCenter.position) + " / " + root.formatTime(VideoCenter.duration)
                        color: "#DFFFFFFF"
                        font.pixelSize: 13
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 14
                    spacing: 14

                    Button {
                        width: 40
                        height: 36
                        hoverEnabled: false
                        contentItem: Label {
                            text: VideoCenter.muted || VideoCenter.volume === 0 ? "×♪" : "♪"
                            color: "#FFFFFF"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 12; color: parent.down ? "#35FFFFFF" : "transparent" }
                        onClicked: root.toggleMediaMute()
                    }

                    ColorSlider {
                        id: theaterVolume
                        width: 110
                        height: 19
                        anchors.verticalCenter: parent.verticalCenter
                        minValue: 0
                        maxValue: 10
                        stepSize: 1
                        value: VideoCenter.volume
                        startColor: "#70A7F5"
                        endColor: "#52E6FB"
                        cornerRadius: 10

                        onValueModified: root.setMediaVolume(value)
                    }

                    Button {
                        width: 90
                        height: 36
                        hoverEnabled: false
                        contentItem: Label {
                            text: qsTr("退出沉浸")
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 13
                            color: parent.down ? "#4A6C9E" : "#31445F"
                            border.width: 1
                            border.color: "#44FFFFFF"
                        }
                        onClicked: root.theaterMode = false
                    }
                }
            }

        }

        // 普通模式底部控制栏
        Rectangle {
            id: normalControlsOverlay
            anchors.left: videoFrame.left
            anchors.right: videoFrame.right
            anchors.top: videoFrame.bottom
            anchors.topMargin: 10
            height: 74
            radius: 16
            color: "#A8070A0F"
            visible: !root.theaterMode
            z: 3

            ColorSlider {
                id: progressSlider
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 8
                height: 19
                minValue: 0
                maxValue: Math.max(1, VideoCenter.duration)
                value: VideoCenter.position
                startColor: "#70A7F5"
                endColor: "#52E6FB"
                cornerRadius: 10

                onValueModified: VideoCenter.seek(value)
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                spacing: 12
                height: 34

                Button {
                    width: 40
                    height: 34
                    hoverEnabled: false
                    contentItem: Label { text: "|◀"; color: "#FFFFFF"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: 12; color: parent.down ? "#405575" : "#26364A" }
                    onClicked: VideoCenter.previous()
                }

                Button {
                    width: 48
                    height: 34
                    hoverEnabled: false
                    contentItem: Label {
                        text: VideoCenter.playing ? "Ⅱ" : "▶"
                        color: "#FFFFFF"
                        font.pixelSize: 17
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 14; color: "#4C79AF" }
                    onClicked: VideoCenter.playPause()
                }

                Button {
                    width: 40
                    height: 34
                    hoverEnabled: false
                    contentItem: Label { text: "▶|"; color: "#FFFFFF"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: 12; color: parent.down ? "#405575" : "#26364A" }
                    onClicked: VideoCenter.next()
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.formatTime(VideoCenter.position) + " / " + root.formatTime(VideoCenter.duration)
                    color: "#DFFFFFFF"
                    font.pixelSize: 12
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                spacing: 12
                height: 34

                Button {
                    width: 38
                    height: 34
                    hoverEnabled: false
                    contentItem: Label {
                        text: VideoCenter.muted || VideoCenter.volume === 0 ? "×♪" : "♪"
                        color: "#FFFFFF"
                        font.pixelSize: 17
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 12; color: parent.down ? "#405575" : "#26364A" }
                    onClicked: root.toggleMediaMute()
                }

                ColorSlider {
                    id: volumeSlider
                    width: 90
                    height: 19
                    anchors.verticalCenter: parent.verticalCenter
                    minValue: 0
                    maxValue: 10
                    stepSize: 1
                    value: VideoCenter.volume
                    startColor: "#70A7F5"
                    endColor: "#52E6FB"
                    cornerRadius: 10

                    onValueModified: root.setMediaVolume(value)
                }

                Button {
                    width: 90
                    height: 34
                    hoverEnabled: false
                    contentItem: Label {
                        text: root.theaterMode ? qsTr("退出沉浸") : qsTr("沉浸播放")
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 13
                        color: parent.down ? "#4A6C9E" : "#31445F"
                        border.width: 1
                        border.color: "#44FFFFFF"
                    }
                    onClicked: root.theaterMode = !root.theaterMode
                }
            }
        }

        Column {
            id: metadataColumn
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: normalControlsOverlay.bottom
            anchors.topMargin: 14
            spacing: 6
            visible: !root.theaterMode

            Row {
                width: parent.width
                spacing: 12
                height: 32

                Label {
                    width: parent.width - categoryBadge.width - singleButton.width - 24
                    text: VideoCenter.title
                    color: "#FFFFFF"
                    font.pixelSize: 21
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    id: categoryBadge
                    width: categoryText.implicitWidth + 22
                    height: 26
                    radius: 13
                    color: VideoCenter.accentColor
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        id: categoryText
                        anchors.centerIn: parent
                        text: VideoCenter.category
                        color: "#EFFFFFFF"
                        font.pixelSize: 11
                    }
                }

                Button {
                    id: singleButton
                    width: 90
                    height: 30
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: false
                    contentItem: Label {
                        text: VideoCenter.autoPlayNext ? qsTr("连续播放") : qsTr("单集播放")
                        color: "#FFFFFF"
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 12
                        color: VideoCenter.autoPlayNext ? "#365A84" : "#26364A"
                        border.width: 1
                        border.color: VideoCenter.autoPlayNext ? "#70A7F5" : "#22FFFFFF"
                    }
                    onClicked: VideoCenter.autoPlayNext = !VideoCenter.autoPlayNext
                }
            }

            Label {
                width: parent.width
                text: VideoCenter.subtitle
                color: "#9FFFFFFF"
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        id: playlistPanel
        x: 924
        y: 116
        width: 438
        height: 580
        radius: 24
        color: "#C9151D28"
        border.width: 1
        border.color: "#24FFFFFF"
        visible: !root.theaterMode

        Label {
            id: playlistTitle
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 18
            text: qsTr("片库 · %1 部").arg(VideoCenter.videoCount)
            color: "#FFFFFF"
            font.pixelSize: 19
            font.weight: Font.DemiBold
        }

        Button {
            id: importVideoButton
            width: 112
            height: 34
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: parent.top
            anchors.topMargin: 13
            enabled: !VideoCenter.importing
            hoverEnabled: false

            contentItem: Label {
                text: VideoCenter.importing ? qsTr("正在导入…") : qsTr("＋ 导入视频")
                color: "#FFFFFF"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 13
                color: importVideoButton.down ? "#4A6C9E" : "#304765"
                border.width: 1
                border.color: "#4C8DFF"
            }

            onClicked: videoImportDialog.open()
        }

        ListView {
            id: playlist
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.top: playlistTitle.bottom
            anchors.topMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            spacing: 12
            clip: true
            model: VideoCenter
            boundsBehavior: Flickable.StopAtBounds

            delegate: VideoListDelegate {
                width: playlist.width
                videoTitle: model.title
                subtitle: model.subtitle
                categoryName: model.categoryName
                durationText: model.durationText
                posterSource: model.posterSource
                accentColor: model.accentColor
                favorite: model.favorite
                current: model.current
                available: model.available
                resumeProgress: model.resumeProgress

                onClicked: VideoCenter.selectVideo(index)
                onFavoriteClicked: VideoCenter.toggleFavorite(index)
                onRestartClicked: VideoCenter.restartVideo(index)
            }
        }
    }

    Timer {
        id: hideControlsTimer
        interval: 5000
        repeat: false
        onTriggered: root.showTheaterControls = false
    }

    Connections {
        target: VideoCenter

        function onCurrentVideoChanged() {
            playlist.positionViewAtIndex(VideoCenter.currentIndex, ListView.Contain)
        }

        function onImportFinished(importedCount, skippedCount, message) {
            Ui.showToast(message)
            if (importedCount > 0)
                playlist.positionViewAtEnd()
        }
    }

    onTheaterModeChanged: {
        if (root.theaterMode) {
            root.showTheaterControls = true
            hideControlsTimer.restart()
        }
    }

    Component.onDestruction: {
        VideoCenter.pause()
        if (root.theaterMode)
            root.theaterMode = false
    }
}
