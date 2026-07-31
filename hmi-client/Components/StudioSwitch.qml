import QtQuick
import QtQuick.Controls

Switch {
    id: root
    implicitWidth: 120
    implicitHeight: 24

    padding: 0
    spacing: 10

    indicator: Rectangle {
        id: indicatorRect          // ← 加 id
        implicitWidth: 40
        implicitHeight: 22
        x: 0
        y: (root.height - height) / 2
        radius: height / 2
        color: root.checked ? "#355D82" : "#243247"
        border.width: 1
        border.color: root.checked ? "#72C4FF" : "#26FFFFFF"

        Behavior on color { ColorAnimation { duration: 140 } }

        Rectangle {
            x: root.visualPosition * (parent.width - width - 4) + 2
            y: (parent.height - height) / 2
            width: 16
            height: 16
            radius: 8
            color: root.checked ? "#FFFFFF" : "#8FFFFFFF"

            Behavior on color { ColorAnimation { duration: 140 } }
            Behavior on x { SmoothedAnimation { duration: 100 } }
        }
    }

    contentItem: Text {
        x: indicatorRect.width + root.spacing   // ← 关键：从滑块右边 + 间距 开始
        width: root.width - x                    // 限制宽度，防止溢出
        text: root.text
        font: root.font
        color: root.enabled ? "#DFFFFFFF" : "#58FFFFFF"
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
