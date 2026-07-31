#ifndef VOICEASSISTANTCONTROLLER_H
#define VOICEASSISTANTCONTROLLER_H

#include "AssistantMessageModel.h"

#include <QAudioFormat>
#include <QElapsedTimer>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QVector>
#include <qqmlintegration.h>

class QAbstractItemModel;
class QJsonObject;
class QUrl;
class QAudioSource;
class QIODevice;
class QWebSocket;

class VoiceAssistantController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(VoiceAssistant)

    Q_PROPERTY(QAbstractItemModel *messages READ messages CONSTANT)
    Q_PROPERTY(QString draftText READ draftText WRITE setDraftText NOTIFY draftTextChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)
    Q_PROPERTY(bool initializing READ initializing NOTIFY initializingChanged)
    Q_PROPERTY(bool finishing READ finishing NOTIFY finishingChanged)
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)
    Q_PROPERTY(bool speechSupported READ speechSupported NOTIFY speechSupportedChanged)
    Q_PROPERTY(bool apiConfigured READ apiConfigured NOTIFY apiConfiguredChanged)
    Q_PROPERTY(QString speechStatus READ speechStatus NOTIFY speechStatusChanged)
    Q_PROPERTY(QString recognitionLanguage READ recognitionLanguage NOTIFY recognitionLanguageChanged)
    Q_PROPERTY(QString liveTranscript READ liveTranscript NOTIFY liveTranscriptChanged)
    Q_PROPERTY(qreal audioLevel READ audioLevel NOTIFY audioLevelChanged)
    Q_PROPERTY(bool speechDetected READ speechDetected NOTIFY speechDetectedChanged)
    Q_PROPERTY(int silenceRemainingMs READ silenceRemainingMs NOTIFY silenceRemainingMsChanged)

    Q_PROPERTY(bool agentConfigured READ agentConfigured NOTIFY agentConfiguredChanged)
    Q_PROPERTY(bool agentConnected READ agentConnected NOTIFY agentConnectedChanged)
    Q_PROPERTY(bool agentBusy READ agentBusy NOTIFY agentBusyChanged)
    Q_PROPERTY(QString agentStatus READ agentStatus NOTIFY agentStatusChanged)
    Q_PROPERTY(QString agentModelName READ agentModelName NOTIFY agentModelNameChanged)
    Q_PROPERTY(QString currentTaskText READ currentTaskText NOTIFY currentTaskTextChanged)

public:
    explicit VoiceAssistantController(QObject *parent = nullptr);
    ~VoiceAssistantController() override;

    QAbstractItemModel *messages();

    QString draftText() const;
    void setDraftText(const QString &text);

    bool listening() const;
    bool initializing() const;
    bool finishing() const;
    bool processing() const;
    bool speechSupported() const;
    bool apiConfigured() const;
    QString speechStatus() const;
    QString recognitionLanguage() const;
    QString liveTranscript() const;
    qreal audioLevel() const;
    bool speechDetected() const;
    int silenceRemainingMs() const;

    bool agentConfigured() const;
    bool agentConnected() const;
    bool agentBusy() const;
    QString agentStatus() const;
    QString agentModelName() const;
    QString currentTaskText() const;

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void startListening();
    Q_INVOKABLE void finishListening();
    Q_INVOKABLE void cancelListening();
    Q_INVOKABLE void stopListening();
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void useQuickPrompt(const QString &text);

    Q_INVOKABLE void reloadAgentConfiguration();
    Q_INVOKABLE void reconnectAgentBackend();
    Q_INVOKABLE void cancelAgentTask();
    Q_INVOKABLE void submitToolResult(const QString &callId,
                                      const QString &toolName,
                                      bool success,
                                      const QString &message,
                                      const QVariantMap &data);

signals:
    void draftTextChanged();
    void listeningChanged();
    void initializingChanged();
    void finishingChanged();
    void processingChanged();
    void speechSupportedChanged();
    void apiConfiguredChanged();
    void speechStatusChanged();
    void recognitionLanguageChanged();
    void liveTranscriptChanged();
    void audioLevelChanged();
    void speechDetectedChanged();
    void silenceRemainingMsChanged();
    void recognitionCompleted(const QString &text);
    void recognitionFailed(const QString &message);

    void agentConfiguredChanged();
    void agentConnectedChanged();
    void agentBusyChanged();
    void agentStatusChanged();
    void agentModelNameChanged();
    void currentTaskTextChanged();
    void agentFailed(const QString &message);
    void toolActionRequested(const QString &callId,
                             const QString &toolName,
                             const QVariantMap &toolArguments);

private:
    void appendWelcomeMessage();
    void loadApiCredentials();
    void refreshSpeechAvailability();

    void setListening(bool value);
    void setInitializing(bool value);
    void setFinishing(bool value);
    void setProcessing(bool value);
    void setSpeechSupported(bool value);
    void setSpeechStatus(const QString &status);
    void setRecognitionLanguage(const QString &language);
    void setLiveTranscript(const QString &text);
    void setAudioLevel(qreal level);
    void setSpeechDetected(bool value);
    void setSilenceRemainingMs(int value);

    void loadAgentConfiguration();
    void connectAgentBackend();
    void scheduleAgentReconnect();
    void sendAgentJson(const QJsonObject &object);
    void handleAgentSocketMessage(const QString &message);
    QUrl agentWebSocketUrl() const;
    void setAgentConfigured(bool value);
    void setAgentConnected(bool value);
    void setAgentBusy(bool value);
    void setAgentStatus(const QString &status);
    void setAgentModelName(const QString &name);
    void setCurrentTaskText(const QString &text);

    QUrl buildAuthorizedUrl() const;
    void openSpeechSession();
    bool startAudioCapture(QString *errorMessage);
    void stopAudioCapture();
    void resetRecognitionSession();
    void abortRecognition(const QString &message, bool notifyUser = true);
    void completeRecognition();

    void readCapturedAudio();
    void convertCapturedAudio(const QByteArray &rawData);
    void appendPcm16Samples(const QByteArray &pcmData);
    void analyzeVoiceActivity(const QByteArray &pcmData);
    void sendNextAudioFrame();
    void sendAudioPacket(int status, const QByteArray &audioData, bool includeSessionParameters);
    void sendFinalPacket();

    void handleSocketMessage(const QString &message);
    void applyRecognitionResult(const QJsonObject &resultObject);
    QString extractResultText(const QJsonObject &resultObject) const;
    QString assembledTranscript() const;

    AssistantMessageModel m_messages;
    QString m_draftText;

    bool m_listening = false;
    bool m_initializing = false;
    bool m_finishing = false;
    bool m_processing = false;
    bool m_speechSupported = false;
    bool m_apiConfigured = false;
    bool m_sessionActive = false;
    bool m_cancelRequested = false;
    bool m_finalPacketSent = false;
    bool m_firstAudioPacketSent = false;
    bool m_resultCommitted = false;
    bool m_speechDetected = false;

    QString m_speechStatus;
    QString m_recognitionLanguage;
    QString m_liveTranscript;
    QString m_appId;
    QString m_apiKey;
    QString m_apiSecret;

    bool m_agentConfigured = false;
    bool m_agentConnected = false;
    bool m_agentBusy = false;
    QString m_agentStatus;
    QString m_agentModelName;
    QString m_currentTaskText;
    QString m_agentBackendUrl;
    QString m_agentSessionId;
    int m_agentReconnectAttempt = 0;

    qreal m_audioLevel = 0.0;
    qreal m_noiseFloor = 0.008;
    int m_silenceRemainingMs = 2000;
    int m_consecutiveVoiceFrames = 0;

    QWebSocket *m_socket = nullptr;
    QWebSocket *m_agentSocket = nullptr;
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioDevice = nullptr;
    QAudioFormat m_captureFormat;

    QByteArray m_captureRemainder;
    QByteArray m_pcmBuffer;
    QVector<float> m_resampleBuffer;
    double m_resamplePosition = 0.0;

    QMap<int, QString> m_resultSegments;
    QElapsedTimer m_sessionElapsed;
    QElapsedTimer m_lastVoiceElapsed;

    QTimer m_frameTimer;
    QTimer m_vadTimer;
    QTimer m_connectionWatchdog;
    QTimer m_finalResultWatchdog;
    QTimer m_agentReconnectTimer;
    QTimer m_agentConnectionWatchdog;
    QTimer m_agentTaskWatchdog;
};

#endif
