import QtQuick
import QtQuick.Controls

// 导航栏
Item {
    id: root

    signal back
    signal home
    signal menu
    signal rotationRequested
    signal split
    signal shutdown


    // 头像
    Button {
        id: portraitButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.top: parent.top
        anchors.topMargin: 24
        hoverEnabled: false

        background: Image {
            id: portraitImage
            anchors.fill: parent
            source: "qrc:/Images/Home/portrait.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: Ui.showToast(qsTr("个人中心当前不可用"))
    }

    // 返回
    Button {
        id: backButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 138
        hoverEnabled: false
        enabled: Ui.canGoBack
        opacity: enabled ? 1.0 : 0.38

        background: Image {
            id: backImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/back.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: {
            back()
            Ui.goBack()
        }
    }

    // 主页
    Button {
        id: homeButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 256
        hoverEnabled: false

        background: Image {
            id: homeImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/home.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: {
            home()
            Ui.goHome()
        }
    }

    // 菜单
    Button {
        id: menuButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 374
        hoverEnabled: false

        background: Image {
            id: menuImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/menu.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: Ui.navigateTo(Ui.PAGE_APP)
    }

    // 旋转
    Button {
        id: rotationButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 492
        hoverEnabled: false

        background: Image {
            id: rotationImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/rotation.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: {
            rotationRequested()
            Ui.toggleScreenRotation()
        }
    }

    // 分屏
    Button {
        id: splitButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 610
        hoverEnabled: false

        background: Image {
            id: splitImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/split.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: Ui.showToast(qsTr("分屏功能当前不可用"))
    }

    // 关机
    Button {
        id: shutdownButton
        width: 68
        height: 68
        anchors.left: parent.left
        anchors.leftMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 728
        hoverEnabled: false

        background: Image {
            id: shutdownImage
            width: 32
            height: 32
            anchors.centerIn: parent
            source: "qrc:/Images/Home/shutdown.png"
            fillMode: Image.PreserveAspectFit
            opacity: parent.down ? 0.6 : 1
        }

        onClicked: shutdown()
    }
}
