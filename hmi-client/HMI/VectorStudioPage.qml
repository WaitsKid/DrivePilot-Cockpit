import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root
    width: 1414
    height: 856
    x: 108
    y: 0

    property point pendingTextPoint: Qt.point(0, 0)
    property bool exportingPng: false
    property var pendingPenSegments: []
    property bool previewPaintScheduled: false
    property bool previewNeedsClear: false
    property rect lastPreviewBounds: Qt.rect(0, 0, 0, 0)
    property rect pendingPenDirtyBounds: Qt.rect(0, 0, 0, 0)
    property var strokePalette: ["#FFFFFF", "#62B6FF", "#72E6C5", "#FFD166", "#FF7B8B", "#B99CFF", "#1A2230"]
    property var fillPalette: ["transparent", "#284A6B80", "#2C7A6480", "#8B6E2B80", "#83384A80", "#5B488680", "#DDE7F080"]

    function ellipsePath(context, x, y, width, height) {
        const kappa = 0.5522848
        const ox = width / 2 * kappa
        const oy = height / 2 * kappa
        const xe = x + width
        const ye = y + height
        const xm = x + width / 2
        const ym = y + height / 2
        context.moveTo(x, ym)
        context.bezierCurveTo(x, ym - oy, xm - ox, y, xm, y)
        context.bezierCurveTo(xm + ox, y, xe, ym - oy, xe, ym)
        context.bezierCurveTo(xe, ym + oy, xm + ox, ye, xm, ye)
        context.bezierCurveTo(xm - ox, ye, x, ym + oy, x, ym)
    }

    function objectBounds(object) {
        const type = object.type
        if (type === "line") {
            return Qt.rect(Math.min(object.x1, object.x2), Math.min(object.y1, object.y2),
                           Math.abs(object.x2 - object.x1), Math.abs(object.y2 - object.y1))
        }
        if (type === "pen") {
            const points = object.points || []
            if (points.length === 0)
                return Qt.rect(0, 0, 0, 0)
            var minX = points[0].x
            var maxX = points[0].x
            var minY = points[0].y
            var maxY = points[0].y
            for (var index = 1; index < points.length; ++index) {
                minX = Math.min(minX, points[index].x)
                maxX = Math.max(maxX, points[index].x)
                minY = Math.min(minY, points[index].y)
                maxY = Math.max(maxY, points[index].y)
            }
            return Qt.rect(minX, minY, maxX - minX, maxY - minY)
        }
        return Qt.rect(object.x || 0, object.y || 0, object.w || 0, object.h || 0)
    }

    function drawGrid(context) {
        context.lineWidth = 1
        context.strokeStyle = "#122F4155"
        context.beginPath()
        for (var gx = 10; gx < drawingCanvas.width; gx += 10) {
            context.moveTo(gx, 0)
            context.lineTo(gx, drawingCanvas.height)
        }
        for (var gy = 10; gy < drawingCanvas.height; gy += 10) {
            context.moveTo(0, gy)
            context.lineTo(drawingCanvas.width, gy)
        }
        context.stroke()

        context.strokeStyle = "#24536C78"
        context.beginPath()
        for (var mx = 50; mx < drawingCanvas.width; mx += 50) {
            context.moveTo(mx, 0)
            context.lineTo(mx, drawingCanvas.height)
        }
        for (var my = 50; my < drawingCanvas.height; my += 50) {
            context.moveTo(0, my)
            context.lineTo(drawingCanvas.width, my)
        }
        context.stroke()
    }

    function drawSelection(context, object) {
        const bounds = root.objectBounds(object)
        context.lineWidth = 2
        context.strokeStyle = "#FFD166"
        context.strokeRect(bounds.x - 6, bounds.y - 6,
                           Math.max(12, bounds.width + 12),
                           Math.max(12, bounds.height + 12))
        context.fillStyle = "#FFD166"
        context.fillRect(bounds.x - 9, bounds.y - 9, 6, 6)
        context.fillRect(bounds.x + bounds.width + 3, bounds.y - 9, 6, 6)
        context.fillRect(bounds.x - 9, bounds.y + bounds.height + 3, 6, 6)
        context.fillRect(bounds.x + bounds.width + 3,
                         bounds.y + bounds.height + 3, 6, 6)
    }

    function drawObject(context, object, selected) {
        if (!object || object.visible === false)
            return

        context.lineWidth = object.strokeWidth || 4
        context.strokeStyle = object.strokeColor || "#62B6FF"
        context.fillStyle = object.fillColor || "transparent"
        context.lineCap = "round"
        context.lineJoin = "round"

        if (object.type === "pen") {
            const points = object.points || []
            if (points.length > 0) {
                context.beginPath()
                context.moveTo(points[0].x, points[0].y)
                for (var pointIndex = 1; pointIndex < points.length; ++pointIndex)
                    context.lineTo(points[pointIndex].x, points[pointIndex].y)
                context.stroke()
            }
        } else if (object.type === "line") {
            context.beginPath()
            context.moveTo(object.x1, object.y1)
            context.lineTo(object.x2, object.y2)
            context.stroke()
        } else if (object.type === "rect") {
            if (object.fillColor !== "transparent")
                context.fillRect(object.x, object.y, object.w, object.h)
            context.strokeRect(object.x, object.y, object.w, object.h)
        } else if (object.type === "ellipse") {
            context.beginPath()
            root.ellipsePath(context, object.x, object.y, object.w, object.h)
            context.closePath()
            if (object.fillColor !== "transparent")
                context.fill()
            context.stroke()
        } else if (object.type === "text") {
            context.font = (object.fontSize || 26) + "px sans-serif"
            context.textAlign = "left"
            context.textBaseline = "top"
            context.fillStyle = object.fillColor === "transparent"
                              ? object.strokeColor : object.fillColor
            context.fillText(object.text || "文字", object.x, object.y)
        }

        if (selected)
            root.drawSelection(context, object)
    }

    function drawStaticScene(context) {
        context.clearRect(0, 0, drawingCanvas.width, drawingCanvas.height)
        context.fillStyle = "#111925"
        context.fillRect(0, 0, drawingCanvas.width, drawingCanvas.height)

        if (VectorStudio.gridVisible && !root.exportingPng)
            root.drawGrid(context)

        const objects = VectorStudio.objects
        const previewSourceIndex = VectorStudio.previewSourceIndex
        for (var index = 0; index < objects.length; ++index) {
            if (index === previewSourceIndex)
                continue
            root.drawObject(context,
                            objects[index],
                            !root.exportingPng && index === VectorStudio.selectedIndex)
        }
    }

    function rectIsEmpty(rectangle) {
        return !rectangle || rectangle.width <= 0 || rectangle.height <= 0
    }

    function uniteRects(first, second) {
        if (root.rectIsEmpty(first))
            return second
        if (root.rectIsEmpty(second))
            return first
        const left = Math.min(first.x, second.x)
        const top = Math.min(first.y, second.y)
        const right = Math.max(first.x + first.width, second.x + second.width)
        const bottom = Math.max(first.y + first.height, second.y + second.height)
        return Qt.rect(left, top, right - left, bottom - top)
    }

    function clampPreviewRect(rectangle) {
        if (root.rectIsEmpty(rectangle))
            return Qt.rect(0, 0, 0, 0)
        const left = Math.max(0, Math.floor(rectangle.x))
        const top = Math.max(0, Math.floor(rectangle.y))
        const right = Math.min(previewCanvas.width,
                               Math.ceil(rectangle.x + rectangle.width))
        const bottom = Math.min(previewCanvas.height,
                                Math.ceil(rectangle.y + rectangle.height))
        return Qt.rect(left, top, Math.max(0, right - left), Math.max(0, bottom - top))
    }

    function previewBounds(object, selected) {
        if (!object || !object.type)
            return Qt.rect(0, 0, 0, 0)
        const bounds = root.objectBounds(object)
        const margin = selected ? 12 : Math.max(4, (object.strokeWidth || 4) / 2 + 3)
        return root.clampPreviewRect(Qt.rect(bounds.x - margin,
                                             bounds.y - margin,
                                             Math.max(1, bounds.width) + margin * 2,
                                             Math.max(1, bounds.height) + margin * 2))
    }

    function segmentBounds(x1, y1, x2, y2) {
        const margin = Math.max(4, VectorStudio.strokeWidth / 2 + 3)
        return root.clampPreviewRect(Qt.rect(Math.min(x1, x2) - margin,
                                             Math.min(y1, y2) - margin,
                                             Math.abs(x2 - x1) + margin * 2,
                                             Math.abs(y2 - y1) + margin * 2))
    }

    function schedulePreviewPaint() {
        if (root.previewPaintScheduled)
            return
        root.previewPaintScheduled = true
        previewCanvas.requestAnimationFrame(function() {
            root.previewPaintScheduled = false

            const object = VectorStudio.previewObject
            const type = object && object.type ? object.type : ""
            var dirtyBounds = root.pendingPenDirtyBounds

            if (type !== "pen") {
                const currentBounds = root.previewBounds(
                                            object,
                                            VectorStudio.previewSourceIndex >= 0)
                dirtyBounds = root.uniteRects(dirtyBounds, root.lastPreviewBounds)
                dirtyBounds = root.uniteRects(dirtyBounds, currentBounds)
            } else if (root.previewNeedsClear) {
                dirtyBounds = root.uniteRects(dirtyBounds, root.lastPreviewBounds)
            }

            if (root.previewNeedsClear && type.length === 0)
                dirtyBounds = root.uniteRects(dirtyBounds, root.lastPreviewBounds)

            dirtyBounds = root.clampPreviewRect(dirtyBounds)
            if (!root.rectIsEmpty(dirtyBounds)) {
                previewCanvas.markDirty(dirtyBounds)
            } else if (root.previewNeedsClear) {
                root.previewNeedsClear = false
                root.lastPreviewBounds = Qt.rect(0, 0, 0, 0)
            }
        })
    }

    function clearPreviewLayer() {
        root.pendingPenSegments = []
        root.pendingPenDirtyBounds = Qt.rect(0, 0, 0, 0)
        root.previewNeedsClear = true
        root.schedulePreviewPaint()
    }

    function queuePenSegment(x1, y1, x2, y2) {
        const dirtyBounds = root.segmentBounds(x1, y1, x2, y2)
        root.pendingPenSegments.push({ "x1": x1, "y1": y1, "x2": x2, "y2": y2 })
        root.pendingPenDirtyBounds = root.uniteRects(root.pendingPenDirtyBounds,
                                                     dirtyBounds)
        root.lastPreviewBounds = root.uniteRects(root.lastPreviewBounds, dirtyBounds)
        root.schedulePreviewPaint()
    }

    function drawPreviewScene(context, region) {
        const object = VectorStudio.previewObject
        const type = object && object.type ? object.type : ""
        const dirtyRegion = root.clampPreviewRect(region)

        if (root.previewNeedsClear || type !== "pen") {
            context.clearRect(dirtyRegion.x,
                              dirtyRegion.y,
                              dirtyRegion.width,
                              dirtyRegion.height)
            root.previewNeedsClear = false
        }

        if (type === "pen") {
            const segments = root.pendingPenSegments
            if (segments.length > 0) {
                context.lineWidth = object.strokeWidth || 4
                context.strokeStyle = object.strokeColor || "#62B6FF"
                context.lineCap = "round"
                context.lineJoin = "round"
                context.beginPath()
                for (var index = 0; index < segments.length; ++index) {
                    const segment = segments[index]
                    context.moveTo(segment.x1, segment.y1)
                    context.lineTo(segment.x2, segment.y2)
                }
                context.stroke()
            }
            root.pendingPenSegments = []
            root.pendingPenDirtyBounds = Qt.rect(0, 0, 0, 0)
            return
        }

        root.pendingPenSegments = []
        root.pendingPenDirtyBounds = Qt.rect(0, 0, 0, 0)
        if (type.length > 0) {
            root.drawObject(context, object, VectorStudio.previewSourceIndex >= 0)
            root.lastPreviewBounds = root.previewBounds(
                                         object,
                                         VectorStudio.previewSourceIndex >= 0)
        } else {
            root.lastPreviewBounds = Qt.rect(0, 0, 0, 0)
        }
    }

    function exportPng() {
        root.exportingPng = true
        drawingCanvas.requestPaint()
        pngExportTimer.restart()
    }

    Image {
        anchors.fill: parent
        source: "qrc:/Images/Home/background.png"
        fillMode: Image.Stretch
    }

    Rectangle {
        anchors.fill: parent
        color: "#26030A15"
    }

    PageChrome { anchors.fill: parent }

    FileDialog {
        id: openDocumentDialog
        title: qsTr("打开 Vector Studio 工程")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Vector Studio 工程 (*.vdraw *.json)"), qsTr("所有文件 (*)")]
        onAccepted: VectorStudio.loadDocument(selectedFile)
    }

    FileDialog {
        id: saveDocumentDialog
        title: qsTr("保存 Vector Studio 工程")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "vdraw"
        nameFilters: [qsTr("Vector Studio 工程 (*.vdraw)")]
        onAccepted: VectorStudio.saveDocumentAs(selectedFile)
    }

    Timer {
        id: pngExportTimer
        interval: 80
        repeat: false
        onTriggered: {
            drawingSurface.grabToImage(function(result) {
                const path = VectorStudio.createPngExportPath()
                const success = result.saveToFile(path)
                VectorStudio.reportPngExport(path, success)
                root.exportingPng = false
                drawingCanvas.requestPaint()
            })
        }
    }

    Connections {
        target: VectorStudio

        function onObjectsChanged() { drawingCanvas.requestPaint() }
        function onSelectionChanged() { drawingCanvas.requestPaint() }
        function onGridVisibleChanged() { drawingCanvas.requestPaint() }
        function onPreviewSourceIndexChanged() { drawingCanvas.requestPaint() }
        function onPreviewObjectChanged() { root.schedulePreviewPaint() }
        function onPenSegmentAdded(x1, y1, x2, y2) {
            root.queuePenSegment(x1, y1, x2, y2)
        }
        function onPreviewCleared() { root.clearPreviewLayer() }
        function onNotification(message) { Ui.showToast(message) }
    }

    Label {
        id: titleLabel
        x: 42
        y: 62
        text: qsTr("Vector Studio")
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }

    Label {
        anchors.left: titleLabel.right
        anchors.leftMargin: 18
        anchors.verticalCenter: titleLabel.verticalCenter
        text: qsTr("%1 · %2 个对象 · %3")
              .arg(VectorStudio.documentName)
              .arg(VectorStudio.objectCount)
              .arg(VectorStudio.saveStatus)
        color: "#8FFFFFFF"
        font.pixelSize: 13
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 108
        anchors.verticalCenter: titleLabel.verticalCenter
        spacing: 8

        StudioButton {
            compact: true
            touch: true
            glyph: "＋"
            text: qsTr("新建")
            onClicked: VectorStudio.newDocument()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "↶"
            text: qsTr("撤销")
            enabled: VectorStudio.canUndo
            onClicked: VectorStudio.undo()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "↷"
            text: qsTr("重做")
            enabled: VectorStudio.canRedo
            onClicked: VectorStudio.redo()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "⌁"
            text: qsTr("打开")
            onClicked: openDocumentDialog.open()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "▣"
            text: qsTr("保存")
            onClicked: saveDocumentDialog.open()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "SVG"
            text: qsTr("导出")
            onClicked: VectorStudio.exportSvg()
        }
        StudioButton {
            compact: true
            touch: true
            glyph: "PNG"
            text: qsTr("导出")
            onClicked: root.exportPng()
        }
    }

    Rectangle {
        id: toolPanel
        x: 32
        y: 116
        width: 148
        height: 580
        radius: 22
        color: "#C916202D"
        border.width: 1
        border.color: "#24FFFFFF"

        Label {
            anchors.top: parent.top
            anchors.topMargin: 18
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("绘图工具")
            color: "#CFFFFFFF"
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 52
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            Repeater {
                model: [
                    { tool: "select", glyph: "⌖", label: qsTr("选择") },
                    { tool: "pen", glyph: "✎", label: qsTr("画笔") },
                    { tool: "line", glyph: "╱", label: qsTr("直线") },
                    { tool: "rect", glyph: "□", label: qsTr("矩形") },
                    { tool: "ellipse", glyph: "○", label: qsTr("椭圆") },
                    { tool: "text", glyph: "T", label: qsTr("文字") },
                    { tool: "eraser", glyph: "⌫", label: qsTr("橡皮") }
                ]

                StudioButton {
                    width: 118
                    height: 43
                    touch: true
                    glyph: modelData.glyph
                    text: modelData.label
                    selected: VectorStudio.activeTool === modelData.tool
                    danger: modelData.tool === "eraser"
                    onClicked: VectorStudio.activeTool = modelData.tool
                }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            spacing: 12

            Row {
                width: parent.width
                spacing: 10
                Label {
                    text: qsTr("显示网格")
                    color: "#CFFFFFFF"
                    font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                StudioSwitch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: VectorStudio.gridVisible
                    onToggled: VectorStudio.gridVisible = checked
                }
            }

            Row {
                width: parent.width
                spacing: 10
                Label {
                    text: qsTr("吸附 10px")
                    color: "#CFFFFFFF"
                    font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                StudioSwitch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: VectorStudio.snapEnabled
                    onToggled: VectorStudio.snapEnabled = checked
                }
            }
        }
    }

    Rectangle {
        id: canvasFrame
        x: 194
        y: 116
        width: 920
        height: 580
        radius: 22
        color: "#B90E151F"
        border.width: 1
        border.color: "#32FFFFFF"

        Rectangle {
            anchors.centerIn: parent
            width: VectorStudio.canvasWidth + 2
            height: VectorStudio.canvasHeight + 2
            color: "#111925"
            border.width: 1
            border.color: "#516F89"

            Item {
                id: drawingSurface
                anchors.centerIn: parent
                width: VectorStudio.canvasWidth
                height: VectorStudio.canvasHeight

                Canvas {
                    id: drawingCanvas
                    anchors.fill: parent
                    renderTarget: Canvas.Image
                    renderStrategy: Canvas.Immediate
                    onPaint: root.drawStaticScene(getContext("2d"))
                }

                Canvas {
                    id: previewCanvas
                    anchors.fill: parent
                    renderTarget: Canvas.Image
                    renderStrategy: Canvas.Immediate
                    onPaint: function(region) {
                        root.drawPreviewScene(getContext("2d"), region)
                    }
                }

                MouseArea {
                    id: drawingMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    preventStealing: true
                    acceptedButtons: Qt.LeftButton
                    cursorShape: VectorStudio.activeTool === "select"
                                 ? Qt.ArrowCursor : Qt.CrossCursor

                    onPressed: function(mouse) {
                        if (VectorStudio.activeTool === "text") {
                            root.pendingTextPoint = Qt.point(mouse.x, mouse.y)
                            textInputOverlay.visible = true
                            textInputField.text = ""
                            textInputField.forceActiveFocus()
                        } else {
                            VectorStudio.pointerPressed(mouse.x, mouse.y)
                        }
                    }
                    onPositionChanged: function(mouse) {
                        if (pressed)
                            VectorStudio.pointerMoved(mouse.x, mouse.y)
                    }
                    onReleased: function(mouse) {
                        if (VectorStudio.activeTool !== "text")
                            VectorStudio.pointerReleased(mouse.x, mouse.y)
                    }
                }
            }
        }
    }

    Rectangle {
        id: inspectorPanel
        x: 1128
        y: 116
        width: 254
        height: 580
        radius: 22
        color: "#C916202D"
        border.width: 1
        border.color: "#24FFFFFF"

        Label {
            x: 18
            y: 16
            text: qsTr("样式与图层")
            color: "#FFFFFF"
            font.pixelSize: 17
            font.weight: Font.DemiBold
        }

        Label {
            x: 18
            y: 54
            text: qsTr("描边")
            color: "#93FFFFFF"
            font.pixelSize: 12
        }

        Row {
            x: 18
            y: 74
            spacing: 7
            Repeater {
                model: root.strokePalette
                Rectangle {
                    width: 24
                    height: 24
                    radius: 12
                    color: modelData
                    border.width: VectorStudio.strokeColor === modelData ? 3 : 1
                    border.color: VectorStudio.strokeColor === modelData ? "#FFFFFF" : "#52FFFFFF"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: VectorStudio.strokeColor = modelData
                    }
                }
            }
        }

        Label {
            x: 18
            y: 108
            text: qsTr("填充")
            color: "#93FFFFFF"
            font.pixelSize: 12
        }

        Row {
            x: 18
            y: 128
            spacing: 7
            Repeater {
                model: root.fillPalette
                Rectangle {
                    width: 24
                    height: 24
                    radius: 6
                    color: modelData === "transparent" ? "#26364A" : modelData
                    border.width: VectorStudio.fillColor === modelData ? 3 : 1
                    border.color: VectorStudio.fillColor === modelData ? "#FFFFFF" : "#52FFFFFF"
                    Label {
                        anchors.centerIn: parent
                        visible: modelData === "transparent"
                        text: "╱"
                        color: "#FF7B8B"
                        font.pixelSize: 18
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: VectorStudio.fillColor = modelData
                    }
                }
            }
        }

        Label {
            x: 18
            y: 165
            text: qsTr("描边宽度  %1 px").arg(Math.round(VectorStudio.strokeWidth))
            color: "#BFFFFFFF"
            font.pixelSize: 12
        }

        StudioSlider {
            x: 16
            y: 184
            width: 222
            from: 1
            to: 18
            stepSize: 1
            value: VectorStudio.strokeWidth
            onMoved: VectorStudio.strokeWidth = value
        }

        TextField {
            id: selectedTextField
            x: 16
            y: 222
            width: 222
            height: 38
            visible: VectorStudio.selectedObject.type === "text"
            text: visible ? VectorStudio.selectedObject.text : ""
            placeholderText: qsTr("编辑文字")
            color: "#FFFFFF"
            selectByMouse: true
            background: Rectangle {
                radius: 12
                color: "#243247"
                border.color: "#35FFFFFF"
            }
            onEditingFinished: VectorStudio.setSelectedText(text)
        }

        Row {
            x: 16
            y: selectedTextField.visible ? 270 : 224
            spacing: 6

            StudioButton {
                compact: true
                touch: true
                width: 52
                glyph: "↑"
                enabled: VectorStudio.selectedIndex >= 0
                onClicked: VectorStudio.bringForward()
            }
            StudioButton {
                compact: true
                touch: true
                width: 52
                glyph: "↓"
                enabled: VectorStudio.selectedIndex >= 0
                onClicked: VectorStudio.sendBackward()
            }
            StudioButton {
                compact: true
                touch: true
                width: 52
                glyph: "⧉"
                enabled: VectorStudio.selectedIndex >= 0
                onClicked: VectorStudio.duplicateSelected()
            }
            StudioButton {
                compact: true
                touch: true
                width: 52
                glyph: "⌫"
                danger: true
                enabled: VectorStudio.selectedIndex >= 0
                onClicked: VectorStudio.deleteSelected()
            }
        }

        Rectangle {
            x: 16
            y: selectedTextField.visible ? 318 : 272
            width: 222
            height: 1
            color: "#24FFFFFF"
        }

        Label {
            x: 18
            y: selectedTextField.visible ? 332 : 286
            text: qsTr("图层（顶部对象优先）")
            color: "#BFFFFFFF"
            font.pixelSize: 12
        }

        ListView {
            id: layerList
            x: 14
            y: selectedTextField.visible ? 354 : 308
            width: 226
            height: selectedTextField.visible ? 206 : 252
            clip: true
            spacing: 5
            model: VectorStudio.objects
            verticalLayoutDirection: ListView.BottomToTop

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: layerList.width
                height: 42
                radius: 11
                color: index === VectorStudio.selectedIndex ? "#355D82" : "#223044"
                border.width: 1
                border.color: index === VectorStudio.selectedIndex ? "#72C4FF" : "#20FFFFFF"

                Label {
                    x: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: 116
                    text: qsTr("%1  %2")
                          .arg(index + 1)
                          .arg(VectorStudio.typeDisplayName(modelData.type))
                    color: modelData.visible === false ? "#58FFFFFF" : "#EFFFFFFF"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                StudioButton {
                    id: visibilityButton
                    touch: true
                    compact: true
                    width: 36
                    height: 30
                    anchors.right: lockButton.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    glyph: modelData.visible === false ? "○" : "●"
                    onClicked: VectorStudio.toggleVisibility(index)
                }

                StudioButton {
                    id: lockButton
                    touch: true
                    compact: true
                    width: 36
                    height: 30
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    glyph: modelData.locked === true ? "▣" : "▢"
                    onClicked: VectorStudio.toggleLocked(index)
                }

                MouseArea {
                    anchors.left: parent.left
                    anchors.right: visibilityButton.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    onClicked: VectorStudio.selectObject(index)
                }
            }

        }
    }

    Rectangle {
        id: textInputOverlay
        anchors.fill: parent
        z: 1000
        visible: false
        color: "#A0000000"

        MouseArea {
            anchors.fill: parent
            onClicked: textInputOverlay.visible = false
        }

        Rectangle {
            width: 420
            height: 190
            anchors.centerIn: parent
            radius: 24
            color: "#F0192534"
            border.width: 1
            border.color: "#4CFFFFFF"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.top: parent.top
                anchors.topMargin: 20
                text: qsTr("添加文字对象")
                color: "#FFFFFF"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            TextField {
                id: textInputField
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.top: parent.top
                anchors.topMargin: 62
                height: 50
                placeholderText: qsTr("输入要放到画布上的文字")
                color: "#FFFFFF"
                selectByMouse: true
                background: Rectangle {
                    radius: 15
                    color: "#243247"
                    border.color: textInputField.activeFocus ? "#72C4FF" : "#35FFFFFF"
                }
                Keys.onReturnPressed: confirmTextButton.clicked()
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                spacing: 10

                StudioButton {
                    compact: true
                    touch: true
                    text: qsTr("取消")
                    onClicked: textInputOverlay.visible = false
                }
                StudioButton {
                    id: confirmTextButton
                    compact: true
                    touch: true
                    text: qsTr("添加")
                    selected: true
                    enabled: textInputField.text.trim().length > 0
                    onClicked: {
                        VectorStudio.addText(root.pendingTextPoint.x,
                                             root.pendingTextPoint.y,
                                             textInputField.text)
                        textInputOverlay.visible = false
                    }
                }
            }
        }
    }
}
