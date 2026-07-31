import QtQuick
import QtQuick.Controls
import DrivePilot

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0

    property int currentTab: 0
    property int editingContactId: -1
    property string editingName: ""
    property string editingPhone: ""

    function openNewContact() {
        editingContactId = -1
        editingName = ""
        editingPhone = ""
        nameField.text = ""
        phoneField.text = ""
        editorOverlay.visible = true
        nameField.forceActiveFocus()
    }

    function openEditContact(contactId, name, phone) {
        editingContactId = contactId
        editingName = name
        editingPhone = phone
        nameField.text = name
        phoneField.text = phone
        editorOverlay.visible = true
        nameField.forceActiveFocus()
    }

    function saveEditorContact() {
        let saved = false
        if (editingContactId < 0)
            saved = PhoneBook.addContact(nameField.text, phoneField.text)
        else
            saved = PhoneBook.updateContact(editingContactId, nameField.text, phoneField.text)

        if (saved)
            editorOverlay.visible = false
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    PageChrome {
        anchors.fill: parent
    }

    Connections {
        target: PhoneBook
        function onNoticeRequested(message) {
            Ui.showToast(message)
        }
    }

    Component.onCompleted: PhoneBook.refresh()

    Label {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 58
        anchors.top: parent.top
        anchors.topMargin: 62
        text: qsTr("电话与联系人")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Label {
        anchors.left: titleLabel.right
        anchors.leftMargin: 16
        anchors.verticalCenter: titleLabel.verticalCenter
        text: qsTr("%1 位联系人 · 本地 SQLite").arg(PhoneBook.contactCount)
        color: "#8FFFFFFF"
        font.pixelSize: 14
    }

    Row {
        id: tabRow
        anchors.left: titleLabel.left
        anchors.top: titleLabel.bottom
        anchors.topMargin: 22
        spacing: 10

        Repeater {
            model: [qsTr("联系人"), qsTr("最近通话"), qsTr("拨号盘")]

            delegate: Button {
                required property string modelData
                required property int index

                width: 116
                height: 40
                hoverEnabled: false
                contentItem: Label {
                    text: modelData
                    color: root.currentTab === index ? "#FFFFFF" : "#A7B3C5"
                    font.pixelSize: 15
                    font.weight: root.currentTab === index ? Font.DemiBold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 20
                    color: root.currentTab === index ? "#496FA3" : "#222B39"
                    border.width: 1
                    border.color: root.currentTab === index ? "#79A9EE" : "#20FFFFFF"
                }
                onClicked: root.currentTab = index
            }
        }
    }

    Rectangle {
        id: contentPanel
        anchors.left: titleLabel.left
        anchors.right: infoPanel.left
        anchors.rightMargin: 40
        anchors.top: tabRow.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 170
        radius: 24
        color: "#B518202C"
        border.width: 1
        border.color: "#24FFFFFF"

        // 联系人
        Item {
            anchors.fill: parent
            visible: root.currentTab === 0

            TextField {
                id: contactSearch
                height: 46
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.right: addContactButton.left
                anchors.rightMargin: 14
                anchors.top: parent.top
                anchors.topMargin: 20
                placeholderText: qsTr("搜索姓名或号码")
                color: "#FFFFFF"
                font.pixelSize: 15
                leftPadding: 18
                rightPadding: 46
                text: PhoneBook.searchText
                selectByMouse: true

                background: Rectangle {
                    radius: 18
                    color: contactSearch.activeFocus ? "#2C3747" : "#222B39"
                    border.width: 1
                    border.color: contactSearch.activeFocus ? "#70A7F5" : "#20FFFFFF"
                }

                onTextEdited: PhoneBook.searchText = text

                Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 42
                    height: parent.height
                    visible: contactSearch.text.length > 0
                    hoverEnabled: false
                    contentItem: Label {
                        text: "×"
                        color: "#9EACC0"
                        font.pixelSize: 23
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Item { }
                    onClicked: PhoneBook.searchText = ""
                }
            }

            Button {
                id: addContactButton
                width: 126
                height: 46
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.verticalCenter: contactSearch.verticalCenter
                hoverEnabled: false
                contentItem: Label {
                    text: qsTr("＋ 新建联系人")
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 18
                    color: addContactButton.down ? "#3D6597" : "#4C79AF"
                }
                onClicked: root.openNewContact()
            }

            ListView {
                id: contactList
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.top: contactSearch.bottom
                anchors.topMargin: 18
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                spacing: 10
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: PhoneBook.contacts

                delegate: ContactCard {
                    width: contactList.width
                    height: 88
                    onCallClicked: PhoneBook.dialContact(model.contactId)
                    onFavoriteClicked: PhoneBook.toggleFavorite(model.contactId)
                    onEditClicked: root.openEditContact(model.contactId, model.name, model.phone)
                }

                ScrollBar.vertical: ScrollBar { }
            }

            Column {
                anchors.centerIn: contactList
                spacing: 10
                visible: contactList.count === 0
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "☏"
                    color: "#5E7087"
                    font.pixelSize: 50
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: PhoneBook.searchText.length > 0
                          ? qsTr("没有找到匹配的联系人")
                          : qsTr("还没有联系人")
                    color: "#B9C5D6"
                    font.pixelSize: 17
                }
            }
        }

        // 最近通话
        Item {
            anchors.fill: parent
            visible: root.currentTab === 1

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.top: parent.top
                anchors.topMargin: 20

                Label {
                    text: qsTr("最近拨号记录")
                    color: "#F2F6FC"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }

                Item { width: parent.width - 246; height: 1 }

                Button {
                    width: 98
                    height: 38
                    hoverEnabled: false
                    contentItem: Label {
                        text: qsTr("清除记录")
                        color: "#D2DAE6"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 16
                        color: parent.down ? "#3D3440" : "#292731"
                        border.width: 1
                        border.color: "#4AFFFFFF"
                    }
                    onClicked: PhoneBook.clearCallHistory()
                }
            }

            ListView {
                id: historyList
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.top: parent.top
                anchors.topMargin: 70
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                spacing: 10
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: PhoneBook.callHistory

                delegate: CallHistoryDelegate {
                    width: historyList.width
                    height: 78
                    onRedialClicked: PhoneBook.dialHistoryNumber(model.displayName, model.phone)
                }

                ScrollBar.vertical: ScrollBar { }
            }

            Column {
                anchors.centerIn: historyList
                spacing: 10
                visible: historyList.count === 0
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "↗"
                    color: "#5E7087"
                    font.pixelSize: 48
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("暂无通话记录")
                    color: "#B9C5D6"
                    font.pixelSize: 17
                }
            }
        }

        // 拨号盘
        Item {
            anchors.fill: parent
            visible: root.currentTab === 2

            Rectangle {
                id: numberDisplay
                width: 408
                height: 72
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 26
                radius: 22
                color: "#222B39"
                border.width: 1
                border.color: "#2DFFFFFF"

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 22
                    anchors.right: backspaceButton.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: PhoneBook.dialNumber.length > 0 ? PhoneBook.dialNumber : qsTr("输入电话号码")
                    color: PhoneBook.dialNumber.length > 0 ? "#FFFFFF" : "#748399"
                    font.pixelSize: 25
                    font.family: "Consolas"
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideLeft
                }

                Button {
                    id: backspaceButton
                    width: 56
                    height: parent.height
                    anchors.right: parent.right
                    hoverEnabled: false
                    contentItem: Label {
                        text: "⌫"
                        color: "#AEBACB"
                        font.pixelSize: 22
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Item { }
                    onClicked: PhoneBook.removeLastDialCharacter()
                    onPressAndHold: PhoneBook.clearDialNumber()
                }
            }

            Grid {
                id: dialGrid
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: numberDisplay.bottom
                anchors.topMargin: 18
                columns: 3
                spacing: 12

                Repeater {
                    model: [
                        { digit: "1", letters: "" },
                        { digit: "2", letters: "ABC" },
                        { digit: "3", letters: "DEF" },
                        { digit: "4", letters: "GHI" },
                        { digit: "5", letters: "JKL" },
                        { digit: "6", letters: "MNO" },
                        { digit: "7", letters: "PQRS" },
                        { digit: "8", letters: "TUV" },
                        { digit: "9", letters: "WXYZ" },
                        { digit: "*", letters: "" },
                        { digit: "0", letters: "+" },
                        { digit: "#", letters: "" }
                    ]

                    delegate: DialPadButton {
                        required property var modelData
                        digit: modelData.digit
                        letters: modelData.letters
                        onDialClicked: function(value) {
                            PhoneBook.appendDialCharacter(value)
                        }
                    }
                }
            }

            Button {
                id: dialButton
                width: 218
                height: 56
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: dialGrid.bottom
                anchors.topMargin: 18
                enabled: PhoneBook.dialNumber.length >= 3
                hoverEnabled: false
                contentItem: Label {
                    text: qsTr("☎  呼叫")
                    color: dialButton.enabled ? "#FFFFFF" : "#8190A4"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 20
                    color: dialButton.enabled
                           ? (dialButton.down ? "#2F8C61" : "#3EAA76")
                           : "#2A3340"
                }
                onClicked: PhoneBook.dialCurrentNumber()
            }

            Button {
                width: 150
                height: 36
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: dialButton.bottom
                anchors.topMargin: 10
                visible: PhoneBook.dialNumber.length >= 3
                hoverEnabled: false
                contentItem: Label {
                    text: qsTr("保存为联系人")
                    color: "#AFC5E2"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Item { }
                onClicked: {
                    root.editingContactId = -1
                    root.editingName = ""
                    root.editingPhone = PhoneBook.dialNumber
                    nameField.text = ""
                    phoneField.text = PhoneBook.dialNumber
                    editorOverlay.visible = true
                    nameField.forceActiveFocus()
                }
            }
        }
    }

    Rectangle {
        id: infoPanel
        width: 342
        anchors.right: parent.right
        anchors.rightMargin: 115
        anchors.top: tabRow.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 170
        radius: 24
        color: "#B518202C"
        border.width: 1
        border.color: "#24FFFFFF"

        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 18

            Label {
                text: qsTr("拨号能力")
                color: "#F2F6FC"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Rectangle {
                width: parent.width
                height: 112
                radius: 18
                color: "#222B39"
                border.width: 1
                border.color: "#20FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8
                    Label {
                        text: "用Qt → tel模拟拨号"
                        color: "#DDE7F5"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: qsTr("Windows 配置“手机连接”等 tel: 处理程序后，可以把号码交给系统发起通话，这都是模拟拨号，Qt 本身不提供蜂窝语音网络。")
                        color: "#92A2B8"
                        font.pixelSize: 13
                        lineHeight: 1.25
                    }
                }
            }

            Label {
                text: qsTr("最近一次操作")
                color: "#AAB7C9"
                font.pixelSize: 14
            }

            Rectangle {
                width: parent.width
                height: 118
                radius: 18
                color: "#222B39"
                border.width: 1
                border.color: PhoneBook.lastDialStatus.length > 0 ? "#407AA9D6" : "#20FFFFFF"

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 7
                    Label {
                        width: parent.width
                        text: PhoneBook.lastDialedName.length > 0
                              ? PhoneBook.lastDialedName
                              : qsTr("尚未拨号")
                        color: "#F1F5FA"
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Label {
                        width: parent.width
                        text: PhoneBook.lastDialedNumber
                        visible: text.length > 0
                        color: "#B4C0D0"
                        font.pixelSize: 15
                        font.family: "Consolas"
                        elide: Text.ElideRight
                    }
                    Label {
                        width: parent.width
                        text: PhoneBook.lastDialStatus.length > 0
                              ? PhoneBook.lastDialStatus
                              : qsTr("拨号结果会显示在这里")
                        color: PhoneBook.lastDialStatus.indexOf("未配置") >= 0
                               ? "#F2A4A4"
                               : "#79D5A4"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // Rectangle {
            //     width: parent.width
            //     height: 148
            //     radius: 18
            //     color: "#202936"
            //     border.width: 1
            //     border.color: "#20FFFFFF"
            //
            //     Column {
            //         anchors.fill: parent
            //         anchors.margins: 16
            //         spacing: 8
            //         Label {
            //             text: qsTr("企业车机中的真实链路")
            //             color: "#DDE7F5"
            //             font.pixelSize: 14
            //             font.weight: Font.DemiBold
            //         }
            //         Label {
            //             width: parent.width
            //             wrapMode: Text.WordWrap
            //             text: qsTr("QML 负责交互；C++ 通过厂商电话服务、蓝牙 HFP 或 Linux D-Bus/ModemManager 调用真实通信栈。Qt 本身不提供蜂窝语音网络。")
            //             color: "#91A1B7"
            //             font.pixelSize: 12
            //             lineHeight: 1.25
            //         }
            //     }
            // }

            // Label {
            //     width: parent.width
            //     text: PhoneBook.databaseReady
            //           ? qsTr("联系人和通话记录已保存到本地 SQLite")
            //           : qsTr("SQLite 数据库初始化失败")
            //     color: PhoneBook.databaseReady ? "#7FD3A6" : "#F2A4A4"
            //     font.pixelSize: 12
            //     wrapMode: Text.WordWrap
            // }
        }
    }

    // 内嵌编辑面板，保持在 designCanvas 中，跟随现有等比缩放。
    Rectangle {
        id: editorOverlay
        anchors.fill: parent
        z: 500
        visible: false
        color: "#A0000000"

        MouseArea {
            anchors.fill: parent
            onClicked: editorOverlay.visible = false
        }

        Rectangle {
            width: 480
            height: root.editingContactId < 0 ? 330 : 390
            anchors.centerIn: parent
            radius: 28
            color: "#1E2734"
            border.width: 1
            border.color: "#4DFFFFFF"

            MouseArea {
                anchors.fill: parent
                onClicked: function(mouse) { mouse.accepted = true }
            }

            Column {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 18

                Label {
                    text: root.editingContactId < 0 ? qsTr("新建联系人") : qsTr("编辑联系人")
                    color: "#FFFFFF"
                    font.pixelSize: 23
                    font.weight: Font.DemiBold
                }

                TextField {
                    id: nameField
                    width: parent.width
                    height: 50
                    text: ""
                    placeholderText: qsTr("姓名")
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    leftPadding: 16
                    selectByMouse: true
                    background: Rectangle {
                        radius: 16
                        color: "#283342"
                        border.width: 1
                        border.color: nameField.activeFocus ? "#70A7F5" : "#24FFFFFF"
                    }
                }

                TextField {
                    id: phoneField
                    width: parent.width
                    height: 50
                    text: ""
                    placeholderText: qsTr("电话号码")
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.family: "Consolas"
                    leftPadding: 16
                    inputMethodHints: Qt.ImhDialableCharactersOnly
                    selectByMouse: true
                    background: Rectangle {
                        radius: 16
                        color: "#283342"
                        border.width: 1
                        border.color: phoneField.activeFocus ? "#70A7F5" : "#24FFFFFF"
                    }
                    Keys.onReturnPressed: root.saveEditorContact()
                }

                Row {
                    width: parent.width
                    spacing: 12

                    Button {
                        width: root.editingContactId < 0 ? 0 : 116
                        height: 46
                        visible: root.editingContactId >= 0
                        hoverEnabled: false
                        contentItem: Label {
                            text: qsTr("删除")
                            color: "#FFD7D7"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 16
                            color: parent.down ? "#7B3E46" : "#63343C"
                        }
                        onClicked: {
                            if (PhoneBook.deleteContact(root.editingContactId))
                                editorOverlay.visible = false
                        }
                    }

                    Item {
                        width: parent.width - cancelButton.width - saveButton.width
                               - (root.editingContactId >= 0 ? 140 : 12)
                        height: 1
                    }

                    Button {
                        id: cancelButton
                        width: 96
                        height: 46
                        hoverEnabled: false
                        contentItem: Label {
                            text: qsTr("取消")
                            color: "#D1DBE8"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 16
                            color: parent.down ? "#364253" : "#2B3543"
                        }
                        onClicked: editorOverlay.visible = false
                    }

                    Button {
                        id: saveButton
                        width: 112
                        height: 46
                        hoverEnabled: false
                        contentItem: Label {
                            text: qsTr("保存")
                            color: "#FFFFFF"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 16
                            color: parent.down ? "#3D6597" : "#4C79AF"
                        }
                        onClicked: root.saveEditorContact()
                    }
                }
            }
        }
    }
}
