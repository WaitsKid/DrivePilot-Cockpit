import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DrivePilot

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0

    function sendCurrentText() {
        if (VoiceAssistant.agentBusy)
            return
        const text = inputArea.text.trim()
        if (text.length === 0)
            return
        VoiceAssistant.sendMessage(text)
        inputArea.text = ""
        inputArea.forceActiveFocus()
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#080E17"
        opacity: 0.22
    }

    Label {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 62
        anchors.top: parent.top
        anchors.topMargin: 66
        text: qsTr("AI Agent")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Label {
        anchors.left: titleLabel.right
        anchors.leftMargin: 18
        anchors.verticalCenter: titleLabel.verticalCenter
        text: qsTr("讯飞语音输入 · WebSocket Agent · 车控工具调用")
        color: "#86A1C3"
        font.pixelSize: 15
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 108
        anchors.verticalCenter: titleLabel.verticalCenter
        spacing: 10

        Button {
            width: 104
            height: 38
            hoverEnabled: false

            contentItem: Label {
                text: VoiceAssistant.agentBusy ? qsTr("取消任务") : qsTr("重连服务")
                color: "#DDE8F7"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 19
                color: parent.down ? "#34445A" : "#253246"
                border.width: 1
                border.color: "#425875"
            }

            onClicked: {
                if (VoiceAssistant.agentBusy)
                    VoiceAssistant.cancelAgentTask()
                else
                    VoiceAssistant.reconnectAgentBackend()
            }
        }

        Button {
            width: 96
            height: 38
            hoverEnabled: false

            contentItem: Label {
                text: qsTr("清空会话")
                color: "#DDE8F7"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 19
                color: parent.down ? "#34445A" : "#253246"
                border.width: 1
                border.color: "#425875"
            }

            onClicked: VoiceAssistant.clearConversation()
        }
    }

    Rectangle {
        id: chatPanel
        width: 862
        height: 570
        anchors.left: parent.left
        anchors.leftMargin: 58
        anchors.top: titleLabel.bottom
        anchors.topMargin: 24
        radius: 24
        color: "#D9141E2B"
        border.width: 1
        border.color: "#30445D"

        Rectangle {
            height: 55
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            radius: 24
            color: "#1E2A3A"

            Rectangle {
                width: parent.width
                height: 26
                anchors.bottom: parent.bottom
                color: parent.color
            }

            Rectangle {
                width: 10
                height: 10
                radius: 5
                anchors.left: parent.left
                anchors.leftMargin: 23
                anchors.verticalCenter: parent.verticalCenter
                color: VoiceAssistant.agentBusy
                       ? "#F5B84C"
                       : (VoiceAssistant.agentConnected ? "#45D99B" : "#E26873")

                SequentialAnimation on opacity {
                    running: VoiceAssistant.processing
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.35; duration: 520 }
                    NumberAnimation { to: 1.0; duration: 520 }
                }
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 45
                anchors.verticalCenter: parent.verticalCenter
                text: VoiceAssistant.agentStatus
                color: "#DCE8F8"
                font.pixelSize: 15
            }

            Label {
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                text: VoiceAssistant.agentConnected
                      ? VoiceAssistant.agentModelName
                      : qsTr("Python 后端离线")
                color: "#8299B7"
                font.pixelSize: 13
            }
        }

        ListView {
            id: messageList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: inputPanel.top
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 68
            anchors.bottomMargin: 14
            clip: true
            spacing: 4
            model: VoiceAssistant.messages
            boundsBehavior: Flickable.StopAtBounds

            delegate: ChatMessageDelegate {
                width: messageList.width
            }

            onCountChanged: Qt.callLater(function() { messageList.positionViewAtEnd() })
        }

        Rectangle {
            id: inputPanel
            height: 92
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.bottomMargin: 18
            radius: 20
            color: "#1B2736"
            border.width: 1
            border.color: inputArea.activeFocus ? "#587FB5" : "#32455E"

            TextArea {
                id: inputArea
                anchors.left: parent.left
                anchors.right: sendButton.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                color: "#F1F5FA"
                placeholderText: VoiceAssistant.agentBusy
                                 ? qsTr("当前任务执行中……")
                                 : qsTr("输入指令或问题，Ctrl+Enter 发送")
                placeholderTextColor: "#6F86A3"
                font.pixelSize: 16
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                background: Item { }

                Keys.onPressed: function(event) {
                    if ((event.modifiers & Qt.ControlModifier)
                            && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
                        root.sendCurrentText()
                        event.accepted = true
                    }
                }
            }

            Button {
                id: sendButton
                width: 90
                height: 52
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                hoverEnabled: false
                enabled: inputArea.text.trim().length > 0
                         && !VoiceAssistant.agentBusy
                         && VoiceAssistant.agentConnected

                contentItem: Label {
                    text: qsTr("发送")
                    color: sendButton.enabled ? "#FFFFFF" : "#73849B"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 18
                    color: sendButton.enabled
                           ? (sendButton.down ? "#3767A8" : "#477DCA")
                           : "#283444"
                }

                onClicked: root.sendCurrentText()
            }
        }
    }

    Rectangle {
        id: voicePanel
        width: 390
        height: 570
        anchors.left: chatPanel.right
        anchors.leftMargin: 24
        anchors.top: chatPanel.top
        radius: 24
        color: "#D9141E2B"
        border.width: 1
        border.color: "#30445D"

        Label {
            anchors.top: parent.top
            anchors.topMargin: 22
            anchors.horizontalCenter: parent.horizontalCenter
            text: VoiceAssistant.finishing
                  ? qsTr("正在完成识别")
                  : (VoiceAssistant.listening
                     ? qsTr("正在聆听")
                     : (VoiceAssistant.initializing ? qsTr("正在连接") : qsTr("点击开始说话")))
            color: "#FFFFFF"
            font.pixelSize: 21
            font.weight: Font.DemiBold
        }

        Item {
            id: microphoneArea
            width: 170
            height: 150
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 57

            Repeater {
                model: 3

                Rectangle {
                    required property int index
                    anchors.centerIn: parent
                    width: 104 + index * 24
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: 2
                    border.color: "#4E86D3"
                    opacity: VoiceAssistant.listening
                             ? Math.max(0.12, 0.28 + VoiceAssistant.audioLevel * 0.55 - index * 0.08)
                             : 0
                    scale: 0.88 + VoiceAssistant.audioLevel * (0.12 + index * 0.04)

                    Behavior on scale {
                        NumberAnimation { duration: 70; easing.type: Easing.OutQuad }
                    }
                    Behavior on opacity {
                        NumberAnimation { duration: 90 }
                    }
                }
            }

            Button {
                id: microphoneButton
                width: 100
                height: 100
                anchors.centerIn: parent
                hoverEnabled: false
                enabled: !VoiceAssistant.finishing
                         && !VoiceAssistant.agentBusy
                         && VoiceAssistant.agentConnected

                contentItem: Column {
                    spacing: 0

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: VoiceAssistant.listening || VoiceAssistant.initializing ? "×" : "●"
                        color: "#FFFFFF"
                        font.pixelSize: VoiceAssistant.listening || VoiceAssistant.initializing ? 38 : 40
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: VoiceAssistant.listening || VoiceAssistant.initializing
                              ? qsTr("取消") : qsTr("说话")
                        color: "#EAF2FD"
                        font.pixelSize: 13
                    }
                }

                background: Rectangle {
                    radius: width / 2
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: VoiceAssistant.listening || VoiceAssistant.initializing
                                   ? "#E25F6B" : "#5E95E4"
                        }
                        GradientStop {
                            position: 1
                            color: VoiceAssistant.listening || VoiceAssistant.initializing
                                   ? "#AA3948" : "#315D9D"
                        }
                    }
                    border.width: 2
                    border.color: "#83B4F5"
                    opacity: microphoneButton.enabled ? 1 : 0.45
                }

                onClicked: {
                    if (VoiceAssistant.listening || VoiceAssistant.initializing)
                        VoiceAssistant.cancelListening()
                    else
                        VoiceAssistant.startListening()
                }
            }
        }

        Button {
            id: finishButton
            width: 174
            height: 42
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: microphoneArea.bottom
            anchors.topMargin: 2
            visible: VoiceAssistant.listening
            enabled: VoiceAssistant.listening
            hoverEnabled: false

            contentItem: Label {
                text: qsTr("完成并发送")
                color: "#FFFFFF"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 17
                color: finishButton.down ? "#2D699B" : "#3987C6"
                border.width: 1
                border.color: "#6CB7E7"
            }

            onClicked: VoiceAssistant.finishListening()
        }

        Rectangle {
            id: transcriptCard
            width: parent.width - 48
            height: 66
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: microphoneArea.bottom
            anchors.topMargin: VoiceAssistant.listening ? 50 : 5
            radius: 15
            color: "#1A2636"
            border.width: 1
            border.color: VoiceAssistant.speechDetected ? "#3D7996" : "#30445D"

            Label {
                anchors.fill: parent
                anchors.margins: 12
                text: VoiceAssistant.liveTranscript.length > 0
                      ? VoiceAssistant.liveTranscript
                      : (VoiceAssistant.listening
                         ? qsTr("识别结果会在这里实时出现……")
                         : qsTr("在项目根目录 config.json 中填写讯飞凭据后即可识别"))
                color: VoiceAssistant.liveTranscript.length > 0 ? "#F0F6FF" : "#7188A5"
                font.pixelSize: 13
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }

        Label {
            id: speechStatusLabel
            width: parent.width - 54
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: transcriptCard.bottom
            anchors.topMargin: 7
            text: VoiceAssistant.speechStatus
            color: VoiceAssistant.apiConfigured ? "#A9BED8" : "#E7B967"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Label {
            width: parent.width - 56
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: speechStatusLabel.bottom
            anchors.topMargin: 3
            text: VoiceAssistant.listening && VoiceAssistant.speechDetected
                  ? qsTr("静默 %1 秒后自动完成")
                        .arg((VoiceAssistant.silenceRemainingMs / 1000.0).toFixed(1))
                  : VoiceAssistant.recognitionLanguage
            color: "#7188A5"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            id: agentStatusCard
            width: parent.width - 48
            height: 61
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 337
            radius: 14
            color: VoiceAssistant.agentBusy ? "#1A3049" : "#1B2736"
            border.width: 1
            border.color: VoiceAssistant.agentBusy ? "#3D6F9F" : "#30445D"

            Rectangle {
                width: 9
                height: 9
                radius: 5
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.top: parent.top
                anchors.topMargin: 14
                color: VoiceAssistant.agentBusy
                       ? "#65A9EE"
                       : (VoiceAssistant.agentConnected ? "#54D2A2" : "#E26873")

                SequentialAnimation on opacity {
                    running: VoiceAssistant.agentBusy
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 480 }
                    NumberAnimation { to: 1.0; duration: 480 }
                }
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 34
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.top: parent.top
                anchors.topMargin: 8
                text: VoiceAssistant.agentStatus
                color: "#E8F2FE"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                text: VoiceAssistant.currentTaskText.length > 0
                      ? VoiceAssistant.currentTaskText
                      : qsTr("等待输入新的车机任务")
                color: "#89A3C1"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Rectangle {
            width: parent.width - 44
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 407
            color: "#304057"
        }

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 421
            text: qsTr("快捷提问")
            color: "#E6EEF8"
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }

        Column {
            width: parent.width - 56
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 451
            spacing: 7

            Repeater {
                model: [
                    qsTr("把空调调到 22 度，切换自动模式，再打开空调页面"),
                    qsTr("播放下一首音乐，把音量调到 45"),
                    qsTr("关闭疲劳驾驶监测并打开车辆设置")
                ]

                Button {
                    required property string modelData
                    width: parent.width
                    height: 32
                    hoverEnabled: false

                    contentItem: Label {
                        text: modelData
                        color: "#DCE7F5"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 14
                    }

                    background: Rectangle {
                        radius: 12
                        color: parent.down ? "#32475F" : "#223044"
                        border.width: 1
                        border.color: "#344A65"
                    }

                    onClicked: VoiceAssistant.useQuickPrompt(modelData)
                }
            }
        }
    }



    Connections {
        target: VoiceAssistant

        function onRecognitionFailed(message) {
            Ui.showToast(message)
        }

    }

    PageChrome {
        anchors.fill: parent
    }
}
