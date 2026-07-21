import QtQuick
import BYD

// 所有常规页面共用的顶部状态栏、底部空调栏和风量弹窗。
// 页面只负责自己的主体内容，公共区域统一在这里维护。
Item {
    id: root

    anchors.fill: parent
    z: 900

    property bool fanPopupVisible: false

    property bool statusBarVisible: true
    property bool climateBarVisible: true

    StatusBar {
        id: statusBar
        width: parent.width
        height: 46
        anchors.left: parent.left
        anchors.top: parent.top
        visible: root.statusBarVisible

        positionStatus: Ui.controlCenterPositionStatus
        bluetoothStatus: Ui.controlCenterBluetoothStatus
        signalStatus: Ui.controlCenterWLANStatus
    }

    ACBar {
        id: climateBar
        width: 1305
        height: 123
        anchors.left: parent.left
        anchors.leftMargin: 55
        anchors.top: parent.top
        anchors.topMargin: 707
        visible: root.climateBarVisible

        onFan: {
            root.fanPopupVisible = !root.fanPopupVisible
            if (root.fanPopupVisible)
                fanPopup.open()
            else
                fanPopup.close()
        }
        onMode: {
            acModePanel.open()
        }
    }

    ACFan {
        id: fanPopup
        onOpened: root.fanPopupVisible = true
        onClosed: root.fanPopupVisible = false
    }

    ACModePanel{
        id: acModePanel
    }

}
