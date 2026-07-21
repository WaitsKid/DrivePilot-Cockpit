#include "VectorStudioController.h"

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QXmlStreamWriter>
#include <QtMath>

namespace {
QString localPathFromUrl(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString normalizedColor(const QString &color, const QString &fallback)
{
    const QString value = color.trimmed();
    return value.isEmpty() ? fallback : value;
}

QString svgPaint(const QString &color)
{
    if (color.compare(QStringLiteral("transparent"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("none");
    const QColor parsed(color);
    return parsed.isValid() ? parsed.name(QColor::HexRgb) : QStringLiteral("#62B6FF");
}

struct TextGeometry
{
    double width = 1.0;
    double height = 1.0;
    double ascent = 1.0;
    double descent = 0.0;
};

TextGeometry measureTextGeometry(const QString &text, double fontSize)
{
    QFont font;
    font.setStyleHint(QFont::SansSerif);
    font.setPixelSize(qMax(1, qRound(fontSize)));

    const QFontMetricsF metrics(font);
    const QRectF tightBounds = metrics.tightBoundingRect(text);

    TextGeometry geometry;
    geometry.width = qMax(
        1.0,
        static_cast<double>(qCeil(qMax(metrics.horizontalAdvance(text),
                                       tightBounds.width()))));
    geometry.height = qMax(1.0, static_cast<double>(qCeil(metrics.height())));
    geometry.ascent = qMax(1.0, static_cast<double>(qCeil(metrics.ascent())));
    geometry.descent = qMax(0.0, static_cast<double>(qCeil(metrics.descent())));
    return geometry;
}

void updateTextGeometry(QVariantMap &object)
{
    const QString text = object.value(QStringLiteral("text")).toString();
    const double fontSize = object.value(QStringLiteral("fontSize"), 26.0).toDouble();
    const TextGeometry geometry = measureTextGeometry(text, fontSize);
    object.insert(QStringLiteral("w"), geometry.width);
    object.insert(QStringLiteral("h"), geometry.height);
    object.insert(QStringLiteral("textAscent"), geometry.ascent);
    object.insert(QStringLiteral("textDescent"), geometry.descent);
}
}

VectorStudioController::VectorStudioController(QObject *parent)
    : QObject(parent)
{
    m_autosaveTimer.setSingleShot(true);
    m_autosaveTimer.setInterval(500);
    connect(&m_autosaveTimer,
            &QTimer::timeout,
            this,
            &VectorStudioController::writeAutosave);
    loadAutosave();
}

QVariantList VectorStudioController::objects() const
{
    return m_objects;
}

int VectorStudioController::objectCount() const
{
    return m_objects.size();
}

int VectorStudioController::selectedIndex() const
{
    return m_selectedIndex;
}

QVariantMap VectorStudioController::selectedObject() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_objects.size())
        return {};
    return m_objects.at(m_selectedIndex).toMap();
}

QVariantMap VectorStudioController::previewObject() const
{
    return m_previewObject;
}

int VectorStudioController::previewSourceIndex() const
{
    return m_previewSourceIndex;
}

QString VectorStudioController::activeTool() const
{
    return m_activeTool;
}

void VectorStudioController::setActiveTool(const QString &tool)
{
    static const QStringList allowed = {
        QStringLiteral("select"),
        QStringLiteral("pen"),
        QStringLiteral("line"),
        QStringLiteral("rect"),
        QStringLiteral("ellipse"),
        QStringLiteral("text"),
        QStringLiteral("eraser")
    };
    const QString normalized = allowed.contains(tool) ? tool : QStringLiteral("select");
    if (m_activeTool == normalized)
        return;
    m_activeTool = normalized;
    emit activeToolChanged();
}

QString VectorStudioController::strokeColor() const
{
    return m_strokeColor;
}

void VectorStudioController::setStrokeColor(const QString &color)
{
    const QString normalized = normalizedColor(color, QStringLiteral("#62B6FF"));
    if (m_strokeColor == normalized && m_selectedIndex < 0)
        return;
    m_strokeColor = normalized;
    if (m_selectedIndex >= 0 && m_selectedIndex < m_objects.size()) {
        beginTransaction();
        QVariantMap object = m_objects.at(m_selectedIndex).toMap();
        object.insert(QStringLiteral("strokeColor"), normalized);
        updateObject(m_selectedIndex, object);
        commitTransaction();
    }
    emit styleChanged();
}

QString VectorStudioController::fillColor() const
{
    return m_fillColor;
}

void VectorStudioController::setFillColor(const QString &color)
{
    const QString normalized = normalizedColor(color, QStringLiteral("transparent"));
    if (m_fillColor == normalized && m_selectedIndex < 0)
        return;
    m_fillColor = normalized;
    if (m_selectedIndex >= 0 && m_selectedIndex < m_objects.size()) {
        beginTransaction();
        QVariantMap object = m_objects.at(m_selectedIndex).toMap();
        object.insert(QStringLiteral("fillColor"), normalized);
        updateObject(m_selectedIndex, object);
        commitTransaction();
    }
    emit styleChanged();
}

double VectorStudioController::strokeWidth() const
{
    return m_strokeWidth;
}

void VectorStudioController::setStrokeWidth(double width)
{
    const double normalized = qBound(1.0, width, 18.0);
    if (qFuzzyCompare(m_strokeWidth, normalized) && m_selectedIndex < 0)
        return;
    m_strokeWidth = normalized;
    if (m_selectedIndex >= 0 && m_selectedIndex < m_objects.size()) {
        beginTransaction();
        QVariantMap object = m_objects.at(m_selectedIndex).toMap();
        object.insert(QStringLiteral("strokeWidth"), normalized);
        updateObject(m_selectedIndex, object);
        commitTransaction();
    }
    emit styleChanged();
}

bool VectorStudioController::gridVisible() const
{
    return m_gridVisible;
}

void VectorStudioController::setGridVisible(bool visible)
{
    if (m_gridVisible == visible)
        return;
    m_gridVisible = visible;
    emit gridVisibleChanged();
}

bool VectorStudioController::snapEnabled() const
{
    return m_snapEnabled;
}

void VectorStudioController::setSnapEnabled(bool enabled)
{
    if (m_snapEnabled == enabled)
        return;
    m_snapEnabled = enabled;
    emit snapEnabledChanged();
}

bool VectorStudioController::canUndo() const
{
    return !m_undoHistory.isEmpty();
}

bool VectorStudioController::canRedo() const
{
    return !m_redoHistory.isEmpty();
}

QString VectorStudioController::documentName() const
{
    return m_documentName;
}

QString VectorStudioController::saveStatus() const
{
    return m_saveStatus;
}

int VectorStudioController::canvasWidth() const
{
    return kCanvasWidth;
}

int VectorStudioController::canvasHeight() const
{
    return kCanvasHeight;
}

double VectorStudioController::snap(double value) const
{
    if (!m_snapEnabled)
        return value;
    return qRound(value / 10.0) * 10.0;
}

void VectorStudioController::pointerPressed(double x, double y)
{
    clearPreview();

    x = qBound(0.0, snap(x), static_cast<double>(kCanvasWidth));
    y = qBound(0.0, snap(y), static_cast<double>(kCanvasHeight));
    m_pressX = x;
    m_pressY = y;
    m_pointerActive = true;
    m_draggingSelection = false;
    m_activeObjectIndex = -1;

    if (m_activeTool == QStringLiteral("select")) {
        const int index = hitTest(x, y);
        selectObject(index);
        if (index >= 0) {
            const QVariantMap object = m_objects.at(index).toMap();
            if (!object.value(QStringLiteral("locked")).toBool()) {
                beginTransaction();
                m_draggingSelection = true;
                m_activeObjectIndex = index;
                m_dragOriginalObject = object;
                setPreviewSourceIndex(index);
                setPreviewObject(object);
            }
        }
        return;
    }

    if (m_activeTool == QStringLiteral("eraser")) {
        const int index = hitTest(x, y);
        if (index >= 0) {
            const QVariantMap object = m_objects.at(index).toMap();
            if (object.value(QStringLiteral("locked")).toBool()) {
                emit notification(QStringLiteral("图层已锁定，无法擦除"));
            } else {
                beginTransaction();
                removeObject(index);
                commitTransaction();
            }
        }
        m_pointerActive = false;
        return;
    }

    if (m_activeTool == QStringLiteral("text")) {
        m_pointerActive = false;
        return;
    }

    beginTransaction();
    selectObject(-1);

    QVariantMap object = createBaseObject(m_activeTool);
    if (m_activeTool == QStringLiteral("pen")) {
        m_activePenPoints.clear();
        m_activePenPoints.append(QPointF(x, y));
    } else if (m_activeTool == QStringLiteral("line")) {
        object.insert(QStringLiteral("x1"), x);
        object.insert(QStringLiteral("y1"), y);
        object.insert(QStringLiteral("x2"), x);
        object.insert(QStringLiteral("y2"), y);
    } else {
        object.insert(QStringLiteral("x"), x);
        object.insert(QStringLiteral("y"), y);
        object.insert(QStringLiteral("w"), 0.0);
        object.insert(QStringLiteral("h"), 0.0);
    }
    setPreviewObject(object);
}

void VectorStudioController::pointerMoved(double x, double y)
{
    if (!m_pointerActive)
        return;

    x = qBound(0.0, snap(x), static_cast<double>(kCanvasWidth));
    y = qBound(0.0, snap(y), static_cast<double>(kCanvasHeight));

    if (m_draggingSelection && m_activeObjectIndex >= 0) {
        QVariantMap object = m_dragOriginalObject;
        translateObject(object, x - m_pressX, y - m_pressY);
        setPreviewObject(object);
        return;
    }

    const QString type = m_previewObject.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("pen")) {
        if (m_activePenPoints.isEmpty()) {
            m_activePenPoints.append(QPointF(x, y));
            return;
        }

        const QPointF previous = m_activePenPoints.constLast();
        const double dx = x - previous.x();
        const double dy = y - previous.y();
        if (dx * dx + dy * dy < 4.0)
            return;

        m_activePenPoints.append(QPointF(x, y));
        emit penSegmentAdded(previous.x(), previous.y(), x, y);
        return;
    }

    QVariantMap object = m_previewObject;
    if (type == QStringLiteral("line")) {
        object.insert(QStringLiteral("x2"), x);
        object.insert(QStringLiteral("y2"), y);
    } else if (type == QStringLiteral("rect") || type == QStringLiteral("ellipse")) {
        object.insert(QStringLiteral("x"), qMin(m_pressX, x));
        object.insert(QStringLiteral("y"), qMin(m_pressY, y));
        object.insert(QStringLiteral("w"), qAbs(x - m_pressX));
        object.insert(QStringLiteral("h"), qAbs(y - m_pressY));
    }
    setPreviewObject(object);
}

void VectorStudioController::pointerReleased(double x, double y)
{
    if (!m_pointerActive)
        return;

    pointerMoved(x, y);
    m_pointerActive = false;

    if (m_draggingSelection) {
        if (m_activeObjectIndex >= 0 && m_activeObjectIndex < m_objects.size()
            && !m_previewObject.isEmpty()) {
            m_objects[m_activeObjectIndex] = m_previewObject;
            emit objectsChanged();
            emit selectionChanged();
        }
    } else if (!m_previewObject.isEmpty()) {
        QVariantMap object = m_previewObject;
        const QString type = object.value(QStringLiteral("type")).toString();
        bool tooSmall = false;

        if (type == QStringLiteral("pen")) {
            tooSmall = m_activePenPoints.size() < 2;
            if (!tooSmall)
                object = finalizedPenObject();
        } else if (type == QStringLiteral("line")) {
            const double dx = object.value(QStringLiteral("x2")).toDouble()
                              - object.value(QStringLiteral("x1")).toDouble();
            const double dy = object.value(QStringLiteral("y2")).toDouble()
                              - object.value(QStringLiteral("y1")).toDouble();
            tooSmall = dx * dx + dy * dy < 9.0;
        } else if (type == QStringLiteral("rect") || type == QStringLiteral("ellipse")) {
            tooSmall = object.value(QStringLiteral("w")).toDouble() < 3.0
                       || object.value(QStringLiteral("h")).toDouble() < 3.0;
        }

        if (!tooSmall) {
            m_objects.append(object);
            m_selectedIndex = m_objects.size() - 1;
            emit objectsChanged();
            emit selectionChanged();
        }
    }

    commitTransaction();
    m_draggingSelection = false;
    m_activeObjectIndex = -1;
    m_dragOriginalObject.clear();
    clearPreview();
}

void VectorStudioController::addText(double x, double y, const QString &text)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty())
        return;

    beginTransaction();
    QVariantMap object = createBaseObject(QStringLiteral("text"));
    object.insert(QStringLiteral("text"), normalized);
    object.insert(QStringLiteral("fontSize"), 26.0);
    object.insert(QStringLiteral("fillColor"), m_strokeColor);
    updateTextGeometry(object);

    const double objectWidth = object.value(QStringLiteral("w")).toDouble();
    const double objectHeight = object.value(QStringLiteral("h")).toDouble();
    object.insert(QStringLiteral("x"),
                  qBound(0.0,
                         snap(x),
                         qMax(0.0, static_cast<double>(kCanvasWidth) - objectWidth)));
    object.insert(QStringLiteral("y"),
                  qBound(0.0,
                         snap(y),
                         qMax(0.0, static_cast<double>(kCanvasHeight) - objectHeight)));

    m_objects.append(object);
    m_selectedIndex = m_objects.size() - 1;
    emit objectsChanged();
    emit selectionChanged();
    commitTransaction();
}

void VectorStudioController::selectObject(int index)
{
    const int normalized = index >= 0 && index < m_objects.size() ? index : -1;
    if (m_selectedIndex == normalized)
        return;
    m_selectedIndex = normalized;
    if (m_selectedIndex >= 0) {
        const QVariantMap object = m_objects.at(m_selectedIndex).toMap();
        m_strokeColor = object.value(QStringLiteral("strokeColor"), m_strokeColor).toString();
        m_fillColor = object.value(QStringLiteral("fillColor"), m_fillColor).toString();
        m_strokeWidth = object.value(QStringLiteral("strokeWidth"), m_strokeWidth).toDouble();
        emit styleChanged();
    }
    emit selectionChanged();
}

void VectorStudioController::deleteSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_objects.size())
        return;
    if (m_objects.at(m_selectedIndex).toMap().value(QStringLiteral("locked")).toBool()) {
        emit notification(QStringLiteral("图层已锁定，无法删除"));
        return;
    }
    beginTransaction();
    removeObject(m_selectedIndex);
    commitTransaction();
}

void VectorStudioController::duplicateSelected()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_objects.size())
        return;
    beginTransaction();
    QVariantMap copy = m_objects.at(m_selectedIndex).toMap();
    copy.insert(QStringLiteral("id"), QDateTime::currentMSecsSinceEpoch());
    translateObject(copy, 18.0, 18.0);
    m_objects.insert(m_selectedIndex + 1, copy);
    ++m_selectedIndex;
    emit objectsChanged();
    emit selectionChanged();
    commitTransaction();
}

void VectorStudioController::bringForward()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_objects.size() - 1)
        return;
    beginTransaction();
    m_objects.swapItemsAt(m_selectedIndex, m_selectedIndex + 1);
    ++m_selectedIndex;
    emit objectsChanged();
    emit selectionChanged();
    commitTransaction();
}

void VectorStudioController::sendBackward()
{
    if (m_selectedIndex <= 0 || m_selectedIndex >= m_objects.size())
        return;
    beginTransaction();
    m_objects.swapItemsAt(m_selectedIndex, m_selectedIndex - 1);
    --m_selectedIndex;
    emit objectsChanged();
    emit selectionChanged();
    commitTransaction();
}

void VectorStudioController::toggleVisibility(int index)
{
    if (index < 0 || index >= m_objects.size())
        return;
    beginTransaction();
    QVariantMap object = m_objects.at(index).toMap();
    object.insert(QStringLiteral("visible"), !object.value(QStringLiteral("visible"), true).toBool());
    updateObject(index, object);
    commitTransaction();
}

void VectorStudioController::toggleLocked(int index)
{
    if (index < 0 || index >= m_objects.size())
        return;
    beginTransaction();
    QVariantMap object = m_objects.at(index).toMap();
    object.insert(QStringLiteral("locked"), !object.value(QStringLiteral("locked"), false).toBool());
    updateObject(index, object);
    commitTransaction();
}

void VectorStudioController::setSelectedText(const QString &text)
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_objects.size())
        return;
    QVariantMap object = m_objects.at(m_selectedIndex).toMap();
    if (object.value(QStringLiteral("type")).toString() != QStringLiteral("text"))
        return;
    beginTransaction();
    object.insert(QStringLiteral("text"), text);
    updateTextGeometry(object);

    const double objectWidth = object.value(QStringLiteral("w")).toDouble();
    const double objectHeight = object.value(QStringLiteral("h")).toDouble();
    object.insert(QStringLiteral("x"),
                  qBound(0.0,
                         object.value(QStringLiteral("x")).toDouble(),
                         qMax(0.0, static_cast<double>(kCanvasWidth) - objectWidth)));
    object.insert(QStringLiteral("y"),
                  qBound(0.0,
                         object.value(QStringLiteral("y")).toDouble(),
                         qMax(0.0, static_cast<double>(kCanvasHeight) - objectHeight)));

    updateObject(m_selectedIndex, object);
    commitTransaction();
}

void VectorStudioController::undo()
{
    if (m_undoHistory.isEmpty())
        return;
    const QByteArray current = serializeScene();
    const QByteArray previous = m_undoHistory.takeLast();
    m_redoHistory.append(current);
    restoreScene(previous);
    trimHistory();
    emit historyChanged();
    scheduleAutosave();
}

void VectorStudioController::redo()
{
    if (m_redoHistory.isEmpty())
        return;
    const QByteArray current = serializeScene();
    const QByteArray next = m_redoHistory.takeLast();
    m_undoHistory.append(current);
    restoreScene(next);
    trimHistory();
    emit historyChanged();
    scheduleAutosave();
}

void VectorStudioController::newDocument()
{
    if (m_objects.isEmpty())
        return;
    beginTransaction();
    m_objects.clear();
    m_selectedIndex = -1;
    m_documentName = QStringLiteral("未命名画布");
    emit objectsChanged();
    emit selectionChanged();
    emit documentChanged();
    commitTransaction();
    emit notification(QStringLiteral("已创建空白画布"));
}

bool VectorStudioController::loadDocument(const QUrl &url)
{
    const QString path = localPathFromUrl(url);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit notification(QStringLiteral("无法打开绘图文件"));
        return false;
    }
    const QByteArray data = file.readAll();
    const QByteArray previous = serializeScene();
    if (!restoreScene(data)) {
        restoreScene(previous);
        emit notification(QStringLiteral("绘图文件格式无效"));
        return false;
    }
    m_undoHistory.append(previous);
    m_redoHistory.clear();
    trimHistory();
    m_documentName = QFileInfo(path).completeBaseName();
    emit documentChanged();
    emit historyChanged();
    scheduleAutosave();
    emit notification(QStringLiteral("绘图工程已载入"));
    return true;
}

bool VectorStudioController::saveDocumentAs(const QUrl &url)
{
    QString path = localPathFromUrl(url);
    if (!path.endsWith(QStringLiteral(".vdraw"), Qt::CaseInsensitive))
        path += QStringLiteral(".vdraw");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(serializeScene()) < 0 || !file.commit()) {
        emit notification(QStringLiteral("保存绘图工程失败"));
        return false;
    }
    m_documentName = QFileInfo(path).completeBaseName();
    emit documentChanged();
    updateSaveStatus(QStringLiteral("工程已保存"));
    emit notification(QStringLiteral("绘图工程已保存"));
    return true;
}

QString VectorStudioController::exportSvg()
{
    const QString path = exportDirectory()
                         + QStringLiteral("/VectorStudio_%1.svg")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit notification(QStringLiteral("无法创建 SVG 文件"));
        return {};
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("svg"));
    xml.writeDefaultNamespace(QStringLiteral("http://www.w3.org/2000/svg"));
    xml.writeAttribute(QStringLiteral("width"), QString::number(kCanvasWidth));
    xml.writeAttribute(QStringLiteral("height"), QString::number(kCanvasHeight));
    xml.writeAttribute(QStringLiteral("viewBox"),
                       QStringLiteral("0 0 %1 %2").arg(kCanvasWidth).arg(kCanvasHeight));
    xml.writeStartElement(QStringLiteral("rect"));
    xml.writeAttribute(QStringLiteral("width"), QStringLiteral("100%"));
    xml.writeAttribute(QStringLiteral("height"), QStringLiteral("100%"));
    xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("#111925"));
    xml.writeEndElement();

    for (const QVariant &value : m_objects) {
        const QVariantMap object = value.toMap();
        if (!object.value(QStringLiteral("visible"), true).toBool())
            continue;
        const QString type = object.value(QStringLiteral("type")).toString();
        const QString stroke = svgPaint(object.value(QStringLiteral("strokeColor"), QStringLiteral("#62B6FF")).toString());
        const QString fill = svgPaint(object.value(QStringLiteral("fillColor"), QStringLiteral("none")).toString());
        const QString width = QString::number(object.value(QStringLiteral("strokeWidth"), 4.0).toDouble());

        if (type == QStringLiteral("pen")) {
            xml.writeStartElement(QStringLiteral("polyline"));
            QStringList points;
            for (const QVariant &pointValue : object.value(QStringLiteral("points")).toList()) {
                const QVariantMap point = pointValue.toMap();
                points.append(QStringLiteral("%1,%2")
                                  .arg(point.value(QStringLiteral("x")).toDouble())
                                  .arg(point.value(QStringLiteral("y")).toDouble()));
            }
            xml.writeAttribute(QStringLiteral("points"), points.join(QLatin1Char(' ')));
            xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
        } else if (type == QStringLiteral("line")) {
            xml.writeStartElement(QStringLiteral("line"));
            xml.writeAttribute(QStringLiteral("x1"), object.value(QStringLiteral("x1")).toString());
            xml.writeAttribute(QStringLiteral("y1"), object.value(QStringLiteral("y1")).toString());
            xml.writeAttribute(QStringLiteral("x2"), object.value(QStringLiteral("x2")).toString());
            xml.writeAttribute(QStringLiteral("y2"), object.value(QStringLiteral("y2")).toString());
            xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
        } else if (type == QStringLiteral("rect")) {
            xml.writeStartElement(QStringLiteral("rect"));
            xml.writeAttribute(QStringLiteral("x"), object.value(QStringLiteral("x")).toString());
            xml.writeAttribute(QStringLiteral("y"), object.value(QStringLiteral("y")).toString());
            xml.writeAttribute(QStringLiteral("width"), object.value(QStringLiteral("w")).toString());
            xml.writeAttribute(QStringLiteral("height"), object.value(QStringLiteral("h")).toString());
            xml.writeAttribute(QStringLiteral("rx"), QStringLiteral("4"));
            xml.writeAttribute(QStringLiteral("fill"), fill);
        } else if (type == QStringLiteral("ellipse")) {
            const double x = object.value(QStringLiteral("x")).toDouble();
            const double y = object.value(QStringLiteral("y")).toDouble();
            const double w = object.value(QStringLiteral("w")).toDouble();
            const double h = object.value(QStringLiteral("h")).toDouble();
            xml.writeStartElement(QStringLiteral("ellipse"));
            xml.writeAttribute(QStringLiteral("cx"), QString::number(x + w / 2.0));
            xml.writeAttribute(QStringLiteral("cy"), QString::number(y + h / 2.0));
            xml.writeAttribute(QStringLiteral("rx"), QString::number(w / 2.0));
            xml.writeAttribute(QStringLiteral("ry"), QString::number(h / 2.0));
            xml.writeAttribute(QStringLiteral("fill"), fill);
        } else if (type == QStringLiteral("text")) {
            const double fontSize = object.value(QStringLiteral("fontSize"), 26.0).toDouble();
            const double ascent = object.value(
                                      QStringLiteral("textAscent"),
                                      measureTextGeometry(
                                          object.value(QStringLiteral("text")).toString(),
                                          fontSize).ascent)
                                      .toDouble();
            xml.writeStartElement(QStringLiteral("text"));
            xml.writeAttribute(QStringLiteral("x"), object.value(QStringLiteral("x")).toString());
            xml.writeAttribute(QStringLiteral("y"),
                               QString::number(object.value(QStringLiteral("y")).toDouble()
                                               + ascent));
            xml.writeAttribute(QStringLiteral("font-size"), QString::number(fontSize));
            xml.writeAttribute(QStringLiteral("font-family"), QStringLiteral("sans-serif"));
            xml.writeAttribute(QStringLiteral("fill"), fill == QStringLiteral("none") ? stroke : fill);
            xml.writeCharacters(object.value(QStringLiteral("text")).toString());
            xml.writeEndElement();
            continue;
        } else {
            continue;
        }
        xml.writeAttribute(QStringLiteral("stroke"), stroke);
        xml.writeAttribute(QStringLiteral("stroke-width"), width);
        xml.writeAttribute(QStringLiteral("stroke-linecap"), QStringLiteral("round"));
        xml.writeAttribute(QStringLiteral("stroke-linejoin"), QStringLiteral("round"));
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();

    if (!file.commit()) {
        emit notification(QStringLiteral("写入 SVG 文件失败"));
        return {};
    }
    emit notification(QStringLiteral("SVG 已导出到图片目录"));
    return path;
}

QString VectorStudioController::createPngExportPath() const
{
    return exportDirectory()
           + QStringLiteral("/VectorStudio_%1.png")
                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
}

void VectorStudioController::reportPngExport(const QString &path, bool success)
{
    emit notification(success
                          ? QStringLiteral("PNG 已导出：%1").arg(QDir::toNativeSeparators(path))
                          : QStringLiteral("PNG 导出失败"));
}

QString VectorStudioController::typeDisplayName(const QString &type) const
{
    if (type == QStringLiteral("pen"))
        return QStringLiteral("自由路径");
    if (type == QStringLiteral("line"))
        return QStringLiteral("直线");
    if (type == QStringLiteral("rect"))
        return QStringLiteral("矩形");
    if (type == QStringLiteral("ellipse"))
        return QStringLiteral("椭圆");
    if (type == QStringLiteral("text"))
        return QStringLiteral("文字");
    return QStringLiteral("对象");
}

int VectorStudioController::hitTest(double x, double y) const
{
    for (int index = m_objects.size() - 1; index >= 0; --index) {
        const QVariantMap object = m_objects.at(index).toMap();
        if (!object.value(QStringLiteral("visible"), true).toBool())
            continue;
        if (hitObject(object, x, y))
            return index;
    }
    return -1;
}

bool VectorStudioController::hitObject(const QVariantMap &object, double x, double y) const
{
    const QString type = object.value(QStringLiteral("type")).toString();
    const double tolerance = qMax(8.0, object.value(QStringLiteral("strokeWidth"), 4.0).toDouble() + 4.0);
    if (type == QStringLiteral("line")) {
        return pointToSegmentDistance(x,
                                      y,
                                      object.value(QStringLiteral("x1")).toDouble(),
                                      object.value(QStringLiteral("y1")).toDouble(),
                                      object.value(QStringLiteral("x2")).toDouble(),
                                      object.value(QStringLiteral("y2")).toDouble()) <= tolerance;
    }
    if (type == QStringLiteral("pen")) {
        const QVariantList points = object.value(QStringLiteral("points")).toList();
        for (int i = 1; i < points.size(); ++i) {
            const QVariantMap first = points.at(i - 1).toMap();
            const QVariantMap second = points.at(i).toMap();
            if (pointToSegmentDistance(x,
                                       y,
                                       first.value(QStringLiteral("x")).toDouble(),
                                       first.value(QStringLiteral("y")).toDouble(),
                                       second.value(QStringLiteral("x")).toDouble(),
                                       second.value(QStringLiteral("y")).toDouble()) <= tolerance)
                return true;
        }
        return false;
    }

    const double objectX = object.value(QStringLiteral("x")).toDouble();
    const double objectY = object.value(QStringLiteral("y")).toDouble();
    const double objectW = object.value(QStringLiteral("w"), 120.0).toDouble();
    const double objectH = object.value(QStringLiteral("h"), 38.0).toDouble();
    if (type == QStringLiteral("ellipse")) {
        if (objectW <= 0.0 || objectH <= 0.0)
            return false;
        const double nx = (x - objectX - objectW / 2.0) / (objectW / 2.0 + tolerance);
        const double ny = (y - objectY - objectH / 2.0) / (objectH / 2.0 + tolerance);
        return nx * nx + ny * ny <= 1.0;
    }
    return x >= objectX - tolerance && x <= objectX + objectW + tolerance
           && y >= objectY - tolerance && y <= objectY + objectH + tolerance;
}

double VectorStudioController::pointToSegmentDistance(double px,
                                                        double py,
                                                        double x1,
                                                        double y1,
                                                        double x2,
                                                        double y2)
{
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy))
        return qSqrt(qPow(px - x1, 2) + qPow(py - y1, 2));
    const double lengthSquared = dx * dx + dy * dy;
    const double t = qBound(0.0, ((px - x1) * dx + (py - y1) * dy) / lengthSquared, 1.0);
    const double closestX = x1 + t * dx;
    const double closestY = y1 + t * dy;
    return qSqrt(qPow(px - closestX, 2) + qPow(py - closestY, 2));
}

QVariantMap VectorStudioController::createBaseObject(const QString &type) const
{
    return QVariantMap{
        {QStringLiteral("id"), QDateTime::currentMSecsSinceEpoch()},
        {QStringLiteral("type"), type},
        {QStringLiteral("strokeColor"), m_strokeColor},
        {QStringLiteral("fillColor"), m_fillColor},
        {QStringLiteral("strokeWidth"), m_strokeWidth},
        {QStringLiteral("visible"), true},
        {QStringLiteral("locked"), false}
    };
}

void VectorStudioController::updateObject(int index, const QVariantMap &object)
{
    if (index < 0 || index >= m_objects.size())
        return;
    m_objects[index] = object;
    emit objectsChanged();
    if (index == m_selectedIndex)
        emit selectionChanged();
}

void VectorStudioController::removeObject(int index)
{
    if (index < 0 || index >= m_objects.size())
        return;
    m_objects.removeAt(index);
    if (m_objects.isEmpty())
        m_selectedIndex = -1;
    else if (m_selectedIndex >= m_objects.size())
        m_selectedIndex = m_objects.size() - 1;
    else if (index < m_selectedIndex)
        --m_selectedIndex;
    emit objectsChanged();
    emit selectionChanged();
}

void VectorStudioController::translateObject(QVariantMap &object, double dx, double dy) const
{
    const QString type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("line")) {
        object.insert(QStringLiteral("x1"), object.value(QStringLiteral("x1")).toDouble() + dx);
        object.insert(QStringLiteral("y1"), object.value(QStringLiteral("y1")).toDouble() + dy);
        object.insert(QStringLiteral("x2"), object.value(QStringLiteral("x2")).toDouble() + dx);
        object.insert(QStringLiteral("y2"), object.value(QStringLiteral("y2")).toDouble() + dy);
    } else if (type == QStringLiteral("pen")) {
        QVariantList points = object.value(QStringLiteral("points")).toList();
        for (QVariant &pointValue : points) {
            QVariantMap point = pointValue.toMap();
            point.insert(QStringLiteral("x"), point.value(QStringLiteral("x")).toDouble() + dx);
            point.insert(QStringLiteral("y"), point.value(QStringLiteral("y")).toDouble() + dy);
            pointValue = point;
        }
        object.insert(QStringLiteral("points"), points);
    } else {
        object.insert(QStringLiteral("x"), object.value(QStringLiteral("x")).toDouble() + dx);
        object.insert(QStringLiteral("y"), object.value(QStringLiteral("y")).toDouble() + dy);
    }
}

void VectorStudioController::setPreviewObject(const QVariantMap &object)
{
    if (m_previewObject == object)
        return;
    m_previewObject = object;
    emit previewObjectChanged();
}

void VectorStudioController::setPreviewSourceIndex(int index)
{
    if (m_previewSourceIndex == index)
        return;
    m_previewSourceIndex = index;
    emit previewSourceIndexChanged();
}

void VectorStudioController::clearPreview()
{
    const bool hadPreview = !m_previewObject.isEmpty()
                            || !m_activePenPoints.isEmpty()
                            || m_previewSourceIndex >= 0;
    m_previewObject.clear();
    m_activePenPoints.clear();
    setPreviewSourceIndex(-1);
    if (hadPreview) {
        emit previewObjectChanged();
        emit previewCleared();
    }
}

QVariantMap VectorStudioController::finalizedPenObject() const
{
    QVariantMap object = m_previewObject;
    QVariantList points;
    points.reserve(m_activePenPoints.size());
    for (const QPointF &point : m_activePenPoints) {
        points.append(QVariantMap{
            {QStringLiteral("x"), point.x()},
            {QStringLiteral("y"), point.y()}
        });
    }
    object.insert(QStringLiteral("points"), points);
    return object;
}

void VectorStudioController::beginTransaction()
{
    if (m_transactionActive)
        return;
    m_transactionSnapshot = serializeScene();
    m_transactionActive = true;
}

void VectorStudioController::commitTransaction()
{
    if (!m_transactionActive)
        return;
    const QByteArray current = serializeScene();
    if (current != m_transactionSnapshot) {
        m_undoHistory.append(m_transactionSnapshot);
        m_redoHistory.clear();
        trimHistory();
        emit historyChanged();
        scheduleAutosave();
    }
    m_transactionSnapshot.clear();
    m_transactionActive = false;
}

QByteArray VectorStudioController::serializeScene() const
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), 2);
    root.insert(QStringLiteral("canvasWidth"), kCanvasWidth);
    root.insert(QStringLiteral("canvasHeight"), kCanvasHeight);
    root.insert(QStringLiteral("objects"), QJsonArray::fromVariantList(m_objects));
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool VectorStudioController::restoreScene(const QByteArray &data)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;
    const QJsonObject rootObject = document.object();
    const int version = rootObject.value(QStringLiteral("version")).toInt(1);
    const QJsonArray array = rootObject.value(QStringLiteral("objects")).toArray();

    QVariantList restoredObjects;
    restoredObjects.reserve(array.size());
    for (const QJsonValue &value : array) {
        QVariantMap object = value.toObject().toVariantMap();
        if (object.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
            const double legacyBaselineY = object.value(QStringLiteral("y")).toDouble();
            updateTextGeometry(object);

            if (version < 2) {
                object.insert(QStringLiteral("y"),
                              legacyBaselineY
                                  - object.value(QStringLiteral("textAscent")).toDouble());
            }

            const double objectWidth = object.value(QStringLiteral("w")).toDouble();
            const double objectHeight = object.value(QStringLiteral("h")).toDouble();
            object.insert(QStringLiteral("x"),
                          qBound(0.0,
                                 object.value(QStringLiteral("x")).toDouble(),
                                 qMax(0.0,
                                      static_cast<double>(kCanvasWidth) - objectWidth)));
            object.insert(QStringLiteral("y"),
                          qBound(0.0,
                                 object.value(QStringLiteral("y")).toDouble(),
                                 qMax(0.0,
                                      static_cast<double>(kCanvasHeight) - objectHeight)));
        }
        restoredObjects.append(object);
    }

    m_objects = restoredObjects;
    m_selectedIndex = m_objects.isEmpty() ? -1 : qMin(m_selectedIndex, m_objects.size() - 1);
    emit objectsChanged();
    emit selectionChanged();
    return true;
}

void VectorStudioController::trimHistory()
{
    while (m_undoHistory.size() > kHistoryLimit)
        m_undoHistory.removeFirst();
    while (m_redoHistory.size() > kHistoryLimit)
        m_redoHistory.removeFirst();
}

QString VectorStudioController::autosaveFilePath() const
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                              + QStringLiteral("/vector_studio");
    QDir().mkpath(directory);
    return directory + QStringLiteral("/autosave.vdraw");
}

QString VectorStudioController::exportDirectory() const
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                        + QStringLiteral("/QtInVehicleHMI");
    if (directory.startsWith(QLatin1Char('/')) && directory.size() <= 2)
        directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                    + QStringLiteral("/exports");
    QDir().mkpath(directory);
    return directory;
}

void VectorStudioController::loadAutosave()
{
    QFile file(autosaveFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (restoreScene(file.readAll())) {
        m_documentName = QStringLiteral("自动恢复画布");
        updateSaveStatus(QStringLiteral("已恢复上次画布"));
        emit documentChanged();
    }
}

void VectorStudioController::scheduleAutosave()
{
    updateSaveStatus(QStringLiteral("正在等待自动保存…"));
    m_autosaveTimer.start();
}

void VectorStudioController::writeAutosave()
{
    QSaveFile file(autosaveFilePath());
    if (!file.open(QIODevice::WriteOnly) || file.write(serializeScene()) < 0 || !file.commit()) {
        updateSaveStatus(QStringLiteral("自动保存失败"));
        return;
    }
    updateSaveStatus(QStringLiteral("已自动保存 %1")
                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))));
}

void VectorStudioController::updateSaveStatus(const QString &status)
{
    if (m_saveStatus == status)
        return;
    m_saveStatus = status;
    emit saveStatusChanged();
}
