#ifndef VECTORSTUDIOCONTROLLER_H
#define VECTORSTUDIOCONTROLLER_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <qqmlintegration.h>

class VectorStudioController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(VectorStudio)

    Q_PROPERTY(QVariantList objects READ objects NOTIFY objectsChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectsChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE selectObject NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedObject READ selectedObject NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap previewObject READ previewObject NOTIFY previewObjectChanged)
    Q_PROPERTY(int previewSourceIndex READ previewSourceIndex NOTIFY previewSourceIndexChanged)
    Q_PROPERTY(QString activeTool READ activeTool WRITE setActiveTool NOTIFY activeToolChanged)
    Q_PROPERTY(QString strokeColor READ strokeColor WRITE setStrokeColor NOTIFY styleChanged)
    Q_PROPERTY(QString fillColor READ fillColor WRITE setFillColor NOTIFY styleChanged)
    Q_PROPERTY(double strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY styleChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY gridVisibleChanged)
    Q_PROPERTY(bool snapEnabled READ snapEnabled WRITE setSnapEnabled NOTIFY snapEnabledChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QString documentName READ documentName NOTIFY documentChanged)
    Q_PROPERTY(QString saveStatus READ saveStatus NOTIFY saveStatusChanged)
    Q_PROPERTY(int canvasWidth READ canvasWidth CONSTANT)
    Q_PROPERTY(int canvasHeight READ canvasHeight CONSTANT)

public:
    explicit VectorStudioController(QObject *parent = nullptr);

    QVariantList objects() const;
    int objectCount() const;
    int selectedIndex() const;
    QVariantMap selectedObject() const;
    QVariantMap previewObject() const;
    int previewSourceIndex() const;

    QString activeTool() const;
    void setActiveTool(const QString &tool);

    QString strokeColor() const;
    void setStrokeColor(const QString &color);
    QString fillColor() const;
    void setFillColor(const QString &color);
    double strokeWidth() const;
    void setStrokeWidth(double width);

    bool gridVisible() const;
    void setGridVisible(bool visible);
    bool snapEnabled() const;
    void setSnapEnabled(bool enabled);

    bool canUndo() const;
    bool canRedo() const;
    QString documentName() const;
    QString saveStatus() const;
    int canvasWidth() const;
    int canvasHeight() const;

    Q_INVOKABLE void pointerPressed(double x, double y);
    Q_INVOKABLE void pointerMoved(double x, double y);
    Q_INVOKABLE void pointerReleased(double x, double y);
    Q_INVOKABLE void addText(double x, double y, const QString &text);

    Q_INVOKABLE void selectObject(int index);
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void duplicateSelected();
    Q_INVOKABLE void bringForward();
    Q_INVOKABLE void sendBackward();
    Q_INVOKABLE void toggleVisibility(int index);
    Q_INVOKABLE void toggleLocked(int index);
    Q_INVOKABLE void setSelectedText(const QString &text);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void newDocument();
    Q_INVOKABLE bool loadDocument(const QUrl &url);
    Q_INVOKABLE bool saveDocumentAs(const QUrl &url);
    Q_INVOKABLE QString exportSvg();
    Q_INVOKABLE QString createPngExportPath() const;
    Q_INVOKABLE void reportPngExport(const QString &path, bool success);
    Q_INVOKABLE QString typeDisplayName(const QString &type) const;

signals:
    void objectsChanged();
    void selectionChanged();
    void previewObjectChanged();
    void previewSourceIndexChanged();
    void penSegmentAdded(double x1, double y1, double x2, double y2);
    void previewCleared();
    void activeToolChanged();
    void styleChanged();
    void gridVisibleChanged();
    void snapEnabledChanged();
    void historyChanged();
    void documentChanged();
    void saveStatusChanged();
    void notification(const QString &message);

private:
    static constexpr int kCanvasWidth = 900;
    static constexpr int kCanvasHeight = 540;
    static constexpr int kHistoryLimit = 60;

    double snap(double value) const;
    int hitTest(double x, double y) const;
    bool hitObject(const QVariantMap &object, double x, double y) const;
    static double pointToSegmentDistance(double px,
                                         double py,
                                         double x1,
                                         double y1,
                                         double x2,
                                         double y2);

    QVariantMap createBaseObject(const QString &type) const;
    void updateObject(int index, const QVariantMap &object);
    void removeObject(int index);
    void translateObject(QVariantMap &object, double dx, double dy) const;
    void setPreviewObject(const QVariantMap &object);
    void setPreviewSourceIndex(int index);
    void clearPreview();
    QVariantMap finalizedPenObject() const;

    void beginTransaction();
    void commitTransaction();
    QByteArray serializeScene() const;
    bool restoreScene(const QByteArray &data);
    void trimHistory();

    QString autosaveFilePath() const;
    QString exportDirectory() const;
    void loadAutosave();
    void scheduleAutosave();
    void writeAutosave();
    void updateSaveStatus(const QString &status);

    QVariantList m_objects;
    int m_selectedIndex = -1;
    QString m_activeTool = QStringLiteral("select");
    QString m_strokeColor = QStringLiteral("#62B6FF");
    QString m_fillColor = QStringLiteral("#284A6B80");
    double m_strokeWidth = 4.0;
    bool m_gridVisible = true;
    bool m_snapEnabled = false;

    QList<QByteArray> m_undoHistory;
    QList<QByteArray> m_redoHistory;
    QByteArray m_transactionSnapshot;
    bool m_transactionActive = false;

    bool m_pointerActive = false;
    bool m_draggingSelection = false;
    int m_activeObjectIndex = -1;
    double m_pressX = 0.0;
    double m_pressY = 0.0;
    QVariantMap m_dragOriginalObject;
    QVariantMap m_previewObject;
    int m_previewSourceIndex = -1;
    QVector<QPointF> m_activePenPoints;

    QString m_documentName = QStringLiteral("未命名画布");
    QString m_saveStatus = QStringLiteral("已准备");
    QTimer m_autosaveTimer;
};

#endif
