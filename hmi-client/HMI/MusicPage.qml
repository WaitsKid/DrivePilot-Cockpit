import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0
    FileDialog {
        id: musicImportDialog
        title: qsTr("导入音乐到车机媒体库")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("音频文件 (*.mp3 *.wav *.m4a *.aac *.flac *.ogg *.opus *.wma)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            var files = []
            for (var index = 0; index < selectedFiles.length; ++index)
                files.push(selectedFiles[index])
            MusicPlayer.importFiles(files)
        }
    }

    function formatTime(milliseconds) {
        var totalSeconds = Math.max(0, Math.floor(milliseconds / 1000))
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        var minuteText = minutes < 10 ? "0" + minutes : minutes.toString()
        var secondText = seconds < 10 ? "0" + seconds : seconds.toString()
        return minuteText + ":" + secondText
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#18030A15"
    }

    PropertyAnimation {
        id: fadeInAnimation
        target: root
        property: "opacity"
        duration: 420
        from: 0
        to: 1
        easing.type: Easing.OutQuad
    }

    Component.onCompleted: fadeInAnimation.start()

    

    Item {
        id: content
        x: 40
        y: 66
        width: parent.width - 80
        height: 616

        Rectangle {
            id: nowPlayingPanel
            width: 470
            height: parent.height
            radius: 28
            color: "#6A111722"
            border.width: 1
            border.color: "#20FFFFFF"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 24
                text: qsTr("正在播放")
                color: "#86FFFFFF"
                font.pixelSize: 16
            }

            AudioVisualizer {
                width: 102
                height: 26
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 22
                active: MusicPlayer.playing
                barColor: MusicPlayer.primaryColor
            }

            MusicCover {
                id: largeCover
                width: 330
                height: 330
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 66
                primaryColor: MusicPlayer.primaryColor
                secondaryColor: MusicPlayer.secondaryColor
                title: MusicPlayer.title
                variant: MusicPlayer.coverVariant
                playing: MusicPlayer.playing
                cornerRadius: 28
            }

            Label {
                width: parent.width - 60
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: largeCover.bottom
                anchors.topMargin: 22
                text: MusicPlayer.title
                color: "#FFFFFF"
                font.pixelSize: 27
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Label {
                width: parent.width - 60
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: largeCover.bottom
                anchors.topMargin: 61
                text: MusicPlayer.artist + " · " + MusicPlayer.album
                color: "#8FFFFFFF"
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 27
                spacing: 28

                PlaybackButton {
                    iconText: "◀◀"
                    accentColor: MusicPlayer.primaryColor
                    onClicked: MusicPlayer.previous()
                }

                PlaybackButton {
                    emphasized: true
                    iconText: MusicPlayer.playing ? "Ⅱ" : "▶"
                    accentColor: MusicPlayer.primaryColor
                    onClicked: MusicPlayer.playPause()
                }

                PlaybackButton {
                    iconText: "▶▶"
                    accentColor: MusicPlayer.primaryColor
                    onClicked: MusicPlayer.next()
                }
            }
        }

        Rectangle {
            id: playerPanel
            anchors.left: nowPlayingPanel.right
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin:108
            height: 252
            radius: 28
            color: "#6A111722"
            border.width: 1
            border.color: "#20FFFFFF"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 24
                text: qsTr("车载音乐")
                color: "#FFFFFF"
                font.pixelSize: 25
                font.bold: true
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 61
                text: qsTr("Qt Multimedia · 本地无版权音频")
                color: "#75FFFFFF"
                font.pixelSize: 14
            }

            Label {
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 28
                text: MusicPlayer.errorString.length > 0
                      ? qsTr("播放异常")
                      : (!MusicPlayer.sourceReady
                         ? qsTr("正在加载")
                         : (MusicPlayer.playing ? qsTr("播放中") : qsTr("已暂停")))
                color: MusicPlayer.errorString.length > 0
                       ? "#FF7B7B"
                       : (MusicPlayer.playing ? MusicPlayer.primaryColor : "#85FFFFFF")
                font.pixelSize: 15
                font.bold: true
            }

            Slider {
                id: progressSlider
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 106
                height: 34
                from: 0
                to: Math.max(1, MusicPlayer.duration)
                enabled: MusicPlayer.seekable
                focusPolicy: Qt.NoFocus

                Binding {
                    target: progressSlider
                    property: "value"
                    value: MusicPlayer.position
                    when: !progressSlider.pressed
                }

                onPressedChanged: {
                    if (!pressed)
                        MusicPlayer.seek(value)
                }

                background: Rectangle {
                    x: progressSlider.leftPadding
                    y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                    width: progressSlider.availableWidth
                    height: 6
                    radius: 3
                    color: "#24FFFFFF"

                    Rectangle {
                        width: progressSlider.visualPosition * parent.width
                        height: parent.height
                        radius: parent.radius
                        color: MusicPlayer.primaryColor
                    }
                }

                handle: Rectangle {
                    x: progressSlider.leftPadding
                       + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                    y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                    width: progressSlider.pressed ? 20 : 16
                    height: width
                    radius: width / 2
                    color: "#F4FFFFFF"
                    border.width: 3
                    border.color: MusicPlayer.primaryColor

                    Behavior on width {
                        NumberAnimation { duration: 100 }
                    }
                }
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: progressSlider.bottom
                anchors.topMargin: 2
                text: root.formatTime(progressSlider.pressed ? progressSlider.value : MusicPlayer.position)
                color: "#7FFFFFFF"
                font.pixelSize: 13
            }

            Label {
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.top: progressSlider.bottom
                anchors.topMargin: 2
                text: root.formatTime(MusicPlayer.duration)
                color: "#7FFFFFFF"
                font.pixelSize: 13
            }

            Button {
                id: muteButton
                width: 44
                height: 44
                anchors.left: parent.left
                anchors.leftMargin: 28
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 18
                hoverEnabled: false

                contentItem: Text {
                    text: MusicPlayer.muted || MusicPlayer.volume === 0 ? "×" : "♪"
                    color: "#FFFFFF"
                    font.pixelSize: 24
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 13
                    color: muteButton.down ? "#36FFFFFF" : "#20FFFFFF"
                }

                onClicked: MusicPlayer.toggleMute()
            }

            Slider {
                id: volumeSlider
                anchors.left: muteButton.right
                anchors.leftMargin: 14
                anchors.right: volumeValue.left
                anchors.rightMargin: 14
                anchors.verticalCenter: muteButton.verticalCenter
                from: 0
                to: 10
                stepSize: 1
                value: 0
                focusPolicy: Qt.NoFocus

                Binding {
                    target: volumeSlider
                    property: "value"
                    value: MusicPlayer.volume
                    when: !volumeSlider.pressed
                }

                onMoved: MusicPlayer.volume = Math.round(value)

                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: volumeSlider.availableWidth
                    height: 6
                    radius: 3
                    color: "#24FFFFFF"

                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height
                        radius: parent.radius
                        color: MusicPlayer.secondaryColor
                    }
                }

                handle: Rectangle {
                    x: volumeSlider.leftPadding
                       + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: 16
                    height: 16
                    radius: 8
                    color: "#F4FFFFFF"
                    border.width: 3
                    border.color: MusicPlayer.secondaryColor
                }
            }

            Label {
                id: volumeValue
                width: 38
                anchors.right: parent.right
                anchors.rightMargin: 28
                anchors.verticalCenter: muteButton.verticalCenter
                text: MusicPlayer.volume
                color: "#CFFFFFFF"
                font.pixelSize: 15
                horizontalAlignment: Text.AlignRight
            }
        }

        Rectangle {
            anchors.left: nowPlayingPanel.right
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin:108
            anchors.top: playerPanel.bottom
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            radius: 28
            color: "#6A111722"
            border.width: 1
            border.color: "#20FFFFFF"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 20
                text: qsTr("播放列表 · %1 首").arg(MusicPlayer.trackCount)
                color: "#FFFFFF"
                font.pixelSize: 21
                font.bold: true
            }

            Button {
                id: importMusicButton
                width: 112
                height: 34
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.top: parent.top
                anchors.topMargin: 15
                enabled: !MusicPlayer.importing
                hoverEnabled: false

                contentItem: Label {
                    text: MusicPlayer.importing ? qsTr("正在导入…") : qsTr("＋ 导入音乐")
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 13
                    color: importMusicButton.down ? "#4A6C9E" : "#304765"
                    border.width: 1
                    border.color: "#4C8DFF"
                }

                onClicked: musicImportDialog.open()
            }

            ListView {
                id: playlistView
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.right: parent.right
                anchors.rightMargin: 18
                anchors.top: parent.top
                anchors.topMargin: 58
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 14
                clip: true
                spacing: 7
                model: MusicPlayer.playlist

                delegate: Item {
                    id: trackDelegate
                    required property var modelData
                    width: playlistView.width
                    height: 76

                    Rectangle {
                        anchors.fill: parent
                        radius: 18
                        color: modelData.index === MusicPlayer.currentIndex
                               ? "#24FFFFFF"
                               : (trackMouseArea.containsMouse ? "#14FFFFFF" : "transparent")
                        border.width: modelData.index === MusicPlayer.currentIndex ? 1 : 0
                        border.color: modelData.primaryColor

                        Behavior on color {
                            ColorAnimation { duration: 120 }
                        }
                    }

                    MusicCover {
                        width: 56
                        height: 56
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        primaryColor: modelData.primaryColor
                        secondaryColor: modelData.secondaryColor
                        title: modelData.title
                        variant: modelData.coverVariant
                        playing: MusicPlayer.playing && modelData.index === MusicPlayer.currentIndex
                        cornerRadius: 14
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 82
                        anchors.top: parent.top
                        anchors.topMargin: 14
                        text: modelData.title
                        color: "#F2FFFFFF"
                        font.pixelSize: 17
                        font.bold: modelData.index === MusicPlayer.currentIndex
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 82
                        anchors.top: parent.top
                        anchors.topMargin: 42
                        text: modelData.artist + " · " + modelData.album
                        color: "#70FFFFFF"
                        font.pixelSize: 13
                    }

                    AudioVisualizer {
                        width: 76
                        height: 24
                        anchors.right: durationLabel.left
                        anchors.rightMargin: 22
                        anchors.verticalCenter: parent.verticalCenter
                        active: MusicPlayer.playing && modelData.index === MusicPlayer.currentIndex
                        visible: modelData.index === MusicPlayer.currentIndex
                        barCount: 10
                        barColor: modelData.primaryColor
                    }

                    Label {
                        id: durationLabel
                        width: 54
                        anchors.right: parent.right
                        anchors.rightMargin: 22
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.durationText
                        color: "#72FFFFFF"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignRight
                    }

                    MouseArea {
                        id: trackMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: MusicPlayer.selectTrack(modelData.index)
                    }
                }

            }
        }
    }

    Connections {
        target: MusicPlayer

        function onImportFinished(importedCount, skippedCount, message) {
            Ui.showToast(message)
            if (importedCount > 0)
                playlistView.positionViewAtEnd()
        }
    }

        // 页面公共区域：状态栏、底部空调栏和自适应风量弹窗
    PageChrome {
        anchors.fill: parent
    }

}
