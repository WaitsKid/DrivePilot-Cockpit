import QtQuick
import QtQuick.Controls
import BYD

Item {
    id: root

    width: 1414
    height: 856
    x: 108
    y: 0

    function launchApp(row, appName, targetPage, available) {
        AppLauncher.markLaunched(row)

        if (available && targetPage > Ui.PAGE_MAIN) {
            Ui.navigateTo(targetPage)
            return
        }

        Ui.showToast(qsTr("%1 为界面演示入口，功能暂未接入").arg(appName))
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
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

    PageChrome {
        anchors.fill: parent
    }

    Label {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 61
        anchors.top: parent.top
        anchors.topMargin: 66
        text: qsTr("应用中心")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Label {
        anchors.left: titleLabel.right
        anchors.leftMargin: 18
        anchors.verticalCenter: titleLabel.verticalCenter
        text: qsTr("%1 个结果").arg(AppLauncher.resultCount)
        color: "#8FFFFFFF"
        font.pixelSize: 15
    }

    TextField {
        id: searchField
        width: 312
        height: 46
        anchors.right: parent.right
        anchors.rightMargin:108
        anchors.verticalCenter: titleLabel.verticalCenter
        placeholderText: qsTr("搜索应用或分类")
        color: "#FFFFFF"
        font.pixelSize: 16
        leftPadding: 18
        rightPadding: clearSearchButton.visible ? 48 : 18
        selectByMouse: true
        text: AppLauncher.searchText
        verticalAlignment: Text.AlignVCenter

        background: Rectangle {
            radius: 23
            color: searchField.activeFocus ? "#2D3544" : "#232B39"
            border.width: 1
            border.color: searchField.activeFocus ? "#78A8FF" : "#24FFFFFF"
        }

        onTextEdited: AppLauncher.searchText = text

        Button {
            id: clearSearchButton
            width: 38
            height: parent.height
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: searchField.text.length > 0
            hoverEnabled: false
            contentItem: Label {
                text: "×"
                color: "#BFFFFFFF"
                font.pixelSize: 25
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Item { }
            onClicked: AppLauncher.searchText = ""
        }
    }

    Row {
        id: actionRow
        anchors.right: searchField.right
        anchors.top: searchField.bottom
        anchors.topMargin: 18
        spacing: 12

        Button {
            id: favoritesButton
            width: 94
            height: 36
            hoverEnabled: false
            contentItem: Label {
                text: AppLauncher.favoritesOnly ? qsTr("★ 收藏") : qsTr("☆ 收藏")
                color: "#FFFFFF"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 18
                color: AppLauncher.favoritesOnly ? "#486A9B" : "#26303E"
                border.width: 1
                border.color: AppLauncher.favoritesOnly ? "#70A7F5" : "#24FFFFFF"
            }
            onClicked: AppLauncher.favoritesOnly = !AppLauncher.favoritesOnly
        }

        Button {
            width: 104
            height: 36
            hoverEnabled: false
            contentItem: Label {
                text: qsTr("常用优先")
                color: "#FFFFFF"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 18
                color: AppLauncher.frequentFirst ? "#486A9B" : "#26303E"
                border.width: 1
                border.color: AppLauncher.frequentFirst ? "#70A7F5" : "#24FFFFFF"
            }
            onClicked: AppLauncher.frequentFirst = !AppLauncher.frequentFirst
        }

        Button {
            width: 76
            height: 36
            hoverEnabled: false
            contentItem: Label {
                text: qsTr("重置")
                color: "#DFFFFFFF"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 18
                color: "#202936"
                border.width: 1
                border.color: "#20FFFFFF"
            }
            onClicked: AppLauncher.clearFilters()
        }
    }

    Flickable {
        id: categoryFlickable
        width: 820
        height: 42
        anchors.left: titleLabel.left
        anchors.top: titleLabel.bottom
        anchors.topMargin: 24
        contentWidth: categoryRow.width
        contentHeight: height
        clip: true
        interactive: contentWidth > width
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: categoryRow
            height: parent.height
            spacing: 10

            Repeater {
                model: AppLauncher.categories

                delegate: Button {
                    required property string modelData

                    width: Math.max(68, categoryText.implicitWidth + 32)
                    height: 36
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: false

                    contentItem: Label {
                        id: categoryText
                        text: modelData
                        color: AppLauncher.category === modelData ? "#FFFFFF" : "#BFFFFFFF"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 18
                        color: AppLauncher.category === modelData ? "#486A9B" : "#222B39"
                        border.width: 1
                        border.color: AppLauncher.category === modelData ? "#70A7F5" : "#20FFFFFF"
                    }

                    onClicked: AppLauncher.category = modelData
                }
            }
        }
    }

    GridView {
        id: appGrid
        anchors.left: parent.left
        anchors.leftMargin: 54
        anchors.right: parent.right
        anchors.rightMargin: 52
        anchors.top: categoryFlickable.bottom
        anchors.topMargin: 17
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 145
        clip: true
        model: AppLauncher
        cellWidth: 184
        cellHeight: 174
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationWraps: true
        highlightMoveDuration: 150

        delegate: AppTile {
            width: 150
            height: 158
            x: (appGrid.cellWidth - width) / 2
            y: 4
            iconSource: model.icon
            appName: model.name
            categoryName: model.categoryName
            favorite: model.favorite
            launchCount: model.launchCount
            available: model.available

            onClicked: root.launchApp(index, model.name, model.targetPage, model.available)
            onFavoriteClicked: AppLauncher.toggleFavorite(index)
        }

        add: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: 180; easing.type: Easing.OutQuad }
            }
        }

        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 180
                easing.type: Easing.OutQuad
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: appGrid.contentHeight > appGrid.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
    }

    Column {
        anchors.centerIn: appGrid
        spacing: 12
        visible: AppLauncher.resultCount === 0

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⌕"
            color: "#72FFFFFF"
            font.pixelSize: 54
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("没有找到匹配的应用")
            color: "#DFFFFFFF"
            font.pixelSize: 20
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("尝试修改搜索词或清除筛选条件")
            color: "#80FFFFFF"
            font.pixelSize: 14
        }
    }
}
