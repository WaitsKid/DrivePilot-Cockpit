import QtQuick
import QtQuick.Controls

Item {
    id: root

    readonly property bool fromUser: model.sender === "user"
    readonly property bool agentStep: model.sender === "agent"
    readonly property bool toolStep: model.sender === "tool"
    readonly property bool systemMessage: model.sender === "system"
    readonly property bool assistantMessage: !fromUser && !agentStep && !toolStep && !systemMessage

    width: ListView.view ? ListView.view.width : 820
    height: processCard.visible ? processCard.height + 10 : bubble.height + 18

    Rectangle {
        id: processCard
        visible: root.agentStep || root.toolStep || root.systemMessage
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 54
        anchors.rightMargin: 54
        height: processText.implicitHeight + 38
        radius: 13
        color: root.agentStep
               ? "#182C43"
               : (root.toolStep ? "#172F2B" : "#252B36")
        border.width: 1
        border.color: root.agentStep
                      ? "#31577E"
                      : (root.toolStep ? "#316458" : "#414A59")

        Rectangle {
            width: 8
            height: 8
            radius: 4
            anchors.left: parent.left
            anchors.leftMargin: 15
            anchors.top: parent.top
            anchors.topMargin: 16
            color: root.agentStep
                   ? "#66A8EE"
                   : (root.toolStep ? "#5BD0A5" : "#A5B1C2")
        }

        Label {
            id: processText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 35
            anchors.rightMargin: 16
            anchors.topMargin: 10
            text: model.messageText
            color: root.agentStep
                   ? "#DCEBFA"
                   : (root.toolStep ? "#D8F4EA" : "#D6DCE5")
            font.pixelSize: 14
            lineHeight: 1.2
            wrapMode: Text.Wrap
        }

        Label {
            anchors.left: processText.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 7
            text: model.messageStatus.length > 0
                  ? qsTr("%1 · %2").arg(model.timeText).arg(model.messageStatus)
                  : model.timeText
            color: "#71859D"
            font.pixelSize: 10
        }
    }

    Rectangle {
        id: avatar
        visible: !processCard.visible
        width: 38
        height: 38
        radius: 19
        anchors.top: bubble.top
        anchors.left: root.fromUser ? undefined : parent.left
        anchors.right: root.fromUser ? parent.right : undefined
        anchors.leftMargin: root.fromUser ? 0 : 8
        anchors.rightMargin: root.fromUser ? 8 : 0
        color: root.fromUser ? "#4777C8" : "#263B5D"
        border.width: 1
        border.color: root.fromUser ? "#78A7F2" : "#4B6998"

        Label {
            anchors.centerIn: parent
            text: root.fromUser ? "我" : "AI"
            color: "#FFFFFF"
            font.pixelSize: root.fromUser ? 14 : 12
            font.weight: Font.DemiBold
        }
    }

    Rectangle {
        id: bubble
        visible: !processCard.visible
        width: Math.min(580, Math.max(170, messageLabel.implicitWidth + 42))
        height: messageLabel.implicitHeight + 48
        anchors.top: parent.top
        anchors.left: root.fromUser ? undefined : avatar.right
        anchors.right: root.fromUser ? avatar.left : undefined
        anchors.leftMargin: root.fromUser ? 0 : 12
        anchors.rightMargin: root.fromUser ? 12 : 0
        radius: 18
        color: root.fromUser ? "#3566AD" : "#202B3A"
        border.width: 1
        border.color: root.fromUser ? "#5688D0" : "#2E3D51"

        Label {
            id: messageLabel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 13
            text: model.messageText
            color: "#F2F6FC"
            font.pixelSize: 16
            lineHeight: 1.25
            wrapMode: Text.Wrap
        }

        Label {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 20
            anchors.bottomMargin: 9
            text: model.messageStatus.length > 0
                  ? qsTr("%1 · %2").arg(model.timeText).arg(model.messageStatus)
                  : model.timeText
            color: root.fromUser ? "#BFD6F8" : "#7F96B4"
            font.pixelSize: 11
        }
    }
}
