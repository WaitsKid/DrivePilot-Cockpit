#ifndef DMSCONTROLLER_H
#define DMSCONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <qqmlintegration.h>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
class QTextToSpeech;
#endif

class DmsController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(DmsSystem)

    Q_PROPERTY(QString endpoint READ endpoint WRITE setEndpoint NOTIFY endpointChanged)
    Q_PROPERTY(bool monitoringEnabled READ monitoringEnabled WRITE setMonitoringEnabled NOTIFY monitoringEnabledChanged)
    Q_PROPERTY(bool clientRunning READ clientRunning NOTIFY clientRunningChanged)
    Q_PROPERTY(bool serviceAvailable READ serviceAvailable NOTIFY statusChanged)
    Q_PROPERTY(bool serviceRunning READ serviceRunning NOTIFY statusChanged)
    Q_PROPERTY(bool modelsReady READ modelsReady NOTIFY statusChanged)
    Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY statusChanged)
    Q_PROPERTY(bool faceDetected READ faceDetected NOTIFY statusChanged)
    Q_PROPERTY(int fatigueLevel READ fatigueLevel NOTIFY statusChanged)
    Q_PROPERTY(QString statusCode READ statusCode NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString monitoringState READ monitoringState NOTIFY statusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY statusChanged)
    Q_PROPERTY(double closedProbability READ closedProbability NOTIFY statusChanged)
    Q_PROPERTY(int closedDurationMs READ closedDurationMs NOTIFY statusChanged)
    Q_PROPERTY(double perclos READ perclos NOTIFY statusChanged)
    Q_PROPERTY(double yawnProbability READ yawnProbability NOTIFY statusChanged)
    Q_PROPERTY(int yawnCountWindow READ yawnCountWindow NOTIFY statusChanged)
    Q_PROPERTY(double processedFps READ processedFps NOTIFY statusChanged)
    Q_PROPERTY(double inferenceMs READ inferenceMs NOTIFY statusChanged)
    Q_PROPERTY(qint64 eventId READ eventId NOTIFY statusChanged)
    Q_PROPERTY(QString eventType READ eventType NOTIFY statusChanged)
    Q_PROPERTY(QString message READ message NOTIFY statusChanged)
    Q_PROPERTY(bool voiceEnabled READ voiceEnabled WRITE setVoiceEnabled NOTIFY voiceEnabledChanged)
    Q_PROPERTY(bool voiceAvailable READ voiceAvailable NOTIFY voiceAvailableChanged)

public:
    explicit DmsController(QObject *parent = nullptr);
    ~DmsController() override;

    QString endpoint() const;
    void setEndpoint(const QString &endpoint);

    bool monitoringEnabled() const;
    void setMonitoringEnabled(bool enabled);
    bool clientRunning() const;
    bool serviceAvailable() const;
    bool serviceRunning() const;
    bool modelsReady() const;
    bool cameraAvailable() const;
    bool faceDetected() const;
    int fatigueLevel() const;
    QString statusCode() const;
    QString statusText() const;
    QString monitoringState() const;
    QString lastError() const;
    double closedProbability() const;
    int closedDurationMs() const;
    double perclos() const;
    double yawnProbability() const;
    int yawnCountWindow() const;
    double processedFps() const;
    double inferenceMs() const;
    qint64 eventId() const;
    QString eventType() const;
    QString message() const;
    bool voiceEnabled() const;
    void setVoiceEnabled(bool enabled);
    bool voiceAvailable() const;

    Q_INVOKABLE void startClient();
    Q_INVOKABLE void stopClient();
    Q_INVOKABLE void refreshNow();
    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();
    Q_INVOKABLE void resetMonitoring();
    Q_INVOKABLE void replayCurrentAlert();

signals:
    void endpointChanged();
    void monitoringEnabledChanged();
    void clientRunningChanged();
    void statusChanged();
    void voiceEnabledChanged();
    void voiceAvailableChanged();
    void alertRequested(const QString &message, int fatigueLevel, qint64 eventId);

private slots:
    void requestStatus();
    void connectWebSocket();
    void handleWebSocketMessage(const QString &message);
    void handleWebSocketConnected();
    void handleWebSocketDisconnected();

private:
    void loadConfiguration();
    QStringList configurationCandidates() const;
    QUrl statusUrl() const;
    QUrl commandUrl(const QString &command) const;
    QUrl webSocketUrl() const;
    void sendCommand(const QString &command, bool applyResponse = true);
    void applyStatusPayload(const QByteArray &data);
    void applyStatusObject(const QJsonObject &object);
    void markServiceUnavailable(const QString &errorText);
    void resetDisabledState();
    void maybeEmitAlert(qint64 backendStartedAtMs,
                        qint64 newEventId,
                        int newFatigueLevel,
                        const QString &newMessage);
    void speak(const QString &text);
    void setClientRunning(bool running);
#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
    void configureSpeechVoice();
    void updateVoiceAvailability();
#endif

    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_statusReply;
    QWebSocket m_webSocket;
    QTimer m_pollTimer;
    QTimer m_reconnectTimer;

    QUrl m_baseUrl;
    bool m_monitoringEnabled = false;
    bool m_clientRunning = false;
    bool m_serviceAvailable = false;
    bool m_serviceRunning = false;
    bool m_modelsReady = false;
    bool m_cameraAvailable = false;
    bool m_faceDetected = false;
    int m_fatigueLevel = 1;
    QString m_statusCode = QStringLiteral("disabled");
    QString m_statusText = QStringLiteral("疲劳驾驶监测已关闭");
    QString m_monitoringState = QStringLiteral("disabled");
    QString m_lastError;
    double m_closedProbability = 0.0;
    int m_closedDurationMs = 0;
    double m_perclos = 0.0;
    double m_yawnProbability = 0.0;
    int m_yawnCountWindow = 0;
    double m_processedFps = 0.0;
    double m_inferenceMs = 0.0;
    qint64 m_eventId = 0;
    QString m_eventType;
    QString m_message;
    qint64 m_backendStartedAtMs = 0;
    qint64 m_lastHandledEventId = 0;
    int m_consecutiveFailures = 0;
    bool m_voiceEnabled = true;
    bool m_voiceAvailable = false;

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
    QTextToSpeech *m_speech = nullptr;
    bool m_speechConfigured = false;
#endif
};

#endif
