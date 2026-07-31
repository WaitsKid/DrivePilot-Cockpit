import QtQuick

Item {
    id: root

    property int weatherCode: -1
    property int sourcePixelSize: 128
    property string accessibleName: ""

    readonly property url iconSource: "qrc:/Images/Weather/" + iconFileName(weatherCode)

    function iconFileName(code) {
        if (code === 0)
            return "sunny.png"
        if (code === 1 || code === 2)
            return "partly_cloudy.png"
        if (code === 3)
            return "cloudy.png"
        if (code === 45 || code === 48)
            return "fog.png"
        if (code === 51 || code === 53 || code === 55 || code === 56 || code === 57)
            return "drizzle.png"
        if (code === 61)
            return "light_rain.png"
        if (code === 63 || code === 66)
            return "moderate_rain.png"
        if (code === 65 || code === 67)
            return "heavy_rain.png"
        if (code === 71)
            return "light_snow.png"
        if (code === 73 || code === 77)
            return "moderate_snow.png"
        if (code === 75)
            return "heavy_snow.png"
        if (code === 80 || code === 81 || code === 82)
            return "rain_shower.png"
        if (code === 85 || code === 86)
            return "snow_shower.png"
        if (code === 95 || code === 96 || code === 99)
            return "thunderstorm.png"
        return "unknown.png"
    }

    Image {
        id: iconImage
        anchors.fill: parent
        source: root.iconSource

        // 不使用 Stretch：不同来源的图标会被强行拉变形。
        // PreserveAspectFit 会把任意原始尺寸安全地缩放进统一显示框。
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        asynchronous: true
        cache: true
        sourceSize.width: root.sourcePixelSize
        sourceSize.height: root.sourcePixelSize
    }

    Text {
        anchors.centerIn: parent
        visible: iconImage.status === Image.Error
        text: "?"
        color: "#BFFFFFFF"
        font.pixelSize: Math.max(16, Math.round(Math.min(root.width, root.height) * 0.45))
        font.weight: Font.DemiBold
    }
}
