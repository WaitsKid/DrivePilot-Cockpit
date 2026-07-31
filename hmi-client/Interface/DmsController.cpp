#include "DmsController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QtGlobal>

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
#include <QTextToSpeech>
#include <QVoice>
#endif

namespace {
constexpr int kPollIntervalMs = 1500;
constexpr int kReconnectIntervalMs = 2500;
constexpr int kRequestTimeoutMs = 1800;
constexpr int kOfflineFailureThreshold = 2;

QString normalizedEndpoint(QString endpoint)
{
    endpoint = endpoint.trimmed();
    if (endpoint.isEmpty())
        endpoint = QStringLiteral("http://127.0.0.1:8765");
    while (endpoint.endsWith(QLatin1Char('/')))
        endpoint.chop(1);
    return endpoint;
}

qint64 jsonInteger(const QJsonObject &object, const QString &key, qint64 fallback = 0)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble())
        return static_cast<qint64>(value.toDouble());
    if (value.isString())
        return value.toString().toLongLong();
    return fallback;
}

}

DmsController::DmsController(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    loadConfiguration();

    QSettings settings;
    m_voiceEnabled = settings.value(QStringLiteral("dms/voiceEnabled"),
                                    m_voiceEnabled).toBool();

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
    const QStringList engines = QTextToSpeech::availableEngines();
    if (!engines.isEmpty()) {
        QString preferredEngine;
#ifdef Q_OS_WIN
        if (engines.contains(QStringLiteral("sapi"), Qt::CaseInsensitive))
            preferredEngine = QStringLiteral("sapi");
        else if (engines.contains(QStringLiteral("winrt"), Qt::CaseInsensitive))
            preferredEngine = QStringLiteral("winrt");
#endif
        m_speech = preferredEngine.isEmpty()
                       ? new QTextToSpeech(this)
                       : new QTextToSpeech(preferredEngine, this);

        connect(m_speech, &QTextToSpeech::stateChanged, this,
                [this](QTextToSpeech::State state) {
            if (state == QTextToSpeech::Ready)
                configureSpeechVoice();
            updateVoiceAvailability();
        });

        connect(m_speech, &QTextToSpeech::errorOccurred, this,
                [this](QTextToSpeech::ErrorReason reason,
                       const QString &errorString) {
            qWarning().noquote() << "[DMS-TTS] 语音错误，reason="
                                 << static_cast<int>(reason)
                                 << "message=" << errorString;
            updateVoiceAvailability();
        });

        connect(m_speech, &QTextToSpeech::engineChanged, this,
                [this](const QString &) {
            m_speechConfigured = false;
            configureSpeechVoice();
            updateVoiceAvailability();
        });

        QTimer::singleShot(0, this, [this]() {
            if (m_speech && m_speech->state() == QTextToSpeech::Ready)
                configureSpeechVoice();
            updateVoiceAvailability();
        });
    } else {
        qWarning().noquote() << "[DMS-TTS] 没有发现可用语音引擎";
        m_voiceAvailable = false;
    }
#else
    m_voiceAvailable = false;
#endif

    m_pollTimer.setInterval(kPollIntervalMs);
    m_pollTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_pollTimer, &QTimer::timeout,
            this, &DmsController::requestStatus);

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(kReconnectIntervalMs);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &DmsController::connectWebSocket);

    connect(&m_webSocket, &QWebSocket::connected,
            this, &DmsController::handleWebSocketConnected);
    connect(&m_webSocket, &QWebSocket::disconnected,
            this, &DmsController::handleWebSocketDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &DmsController::handleWebSocketMessage);
}

DmsController::~DmsController()
{
    stopClient();
}

QString DmsController::endpoint() const
{
    return m_baseUrl.toString();
}

void DmsController::setEndpoint(const QString &endpoint)
{
    const QUrl newUrl(normalizedEndpoint(endpoint));
    if (!newUrl.isValid() || newUrl.scheme().isEmpty())
        return;
    if (m_baseUrl == newUrl)
        return;

    const bool restart = m_clientRunning;
    if (restart)
        stopClient();

    m_baseUrl = newUrl;
    emit endpointChanged();

    if (restart)
        startClient();
}

bool DmsController::monitoringEnabled() const
{
    return m_monitoringEnabled;
}

void DmsController::setMonitoringEnabled(bool enabled)
{
    if (m_monitoringEnabled == enabled) {
        if (enabled && !m_clientRunning) {
            startClient();
            startMonitoring();
        }
        return;
    }

    m_monitoringEnabled = enabled;
    emit monitoringEnabledChanged();

    if (enabled) {
        m_statusCode = QStringLiteral("connecting");
        m_statusText = QStringLiteral("疲劳监测服务连接中");
        m_monitoringState = QStringLiteral("connecting");
        m_lastError.clear();
        emit statusChanged();
        startClient();
        startMonitoring();
        return;
    }

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
    if (m_speech)
        m_speech->stop();
#endif

    if (m_clientRunning || m_serviceAvailable)
        sendCommand(QStringLiteral("stop"), false);
    stopClient();
    resetDisabledState();
}

bool DmsController::clientRunning() const { return m_clientRunning; }
bool DmsController::serviceAvailable() const { return m_serviceAvailable; }
bool DmsController::serviceRunning() const { return m_serviceRunning; }
bool DmsController::modelsReady() const { return m_modelsReady; }
bool DmsController::cameraAvailable() const { return m_cameraAvailable; }
bool DmsController::faceDetected() const { return m_faceDetected; }
int DmsController::fatigueLevel() const { return m_fatigueLevel; }
QString DmsController::statusCode() const { return m_statusCode; }
QString DmsController::statusText() const { return m_statusText; }
QString DmsController::monitoringState() const { return m_monitoringState; }
QString DmsController::lastError() const { return m_lastError; }
double DmsController::closedProbability() const { return m_closedProbability; }
int DmsController::closedDurationMs() const { return m_closedDurationMs; }
double DmsController::perclos() const { return m_perclos; }
double DmsController::yawnProbability() const { return m_yawnProbability; }
int DmsController::yawnCountWindow() const { return m_yawnCountWindow; }
double DmsController::processedFps() const { return m_processedFps; }
double DmsController::inferenceMs() const { return m_inferenceMs; }
qint64 DmsController::eventId() const { return m_eventId; }
QString DmsController::eventType() const { return m_eventType; }
QString DmsController::message() const { return m_message; }
bool DmsController::voiceEnabled() const { return m_voiceEnabled; }
bool DmsController::voiceAvailable() const { return m_voiceAvailable; }

void DmsController::setVoiceEnabled(bool enabled)
{
    if (m_voiceEnabled == enabled)
        return;
    m_voiceEnabled = enabled;
    QSettings settings;
    settings.setValue(QStringLiteral("dms/voiceEnabled"), m_voiceEnabled);
    emit voiceEnabledChanged();
}

void DmsController::startClient()
{
    if (!m_monitoringEnabled || m_clientRunning)
        return;
    setClientRunning(true);
    m_pollTimer.start();
    requestStatus();
    connectWebSocket();
}

void DmsController::stopClient()
{
    if (!m_clientRunning)
        return;
    m_pollTimer.stop();
    m_reconnectTimer.stop();
    if (m_statusReply)
        m_statusReply->abort();
    m_webSocket.close();
    setClientRunning(false);
}

void DmsController::refreshNow()
{
    if (!m_monitoringEnabled)
        return;
    requestStatus();
    if (m_clientRunning && m_webSocket.state() == QAbstractSocket::UnconnectedState)
        connectWebSocket();
}

void DmsController::startMonitoring()
{
    if (!m_monitoringEnabled)
        return;
    sendCommand(QStringLiteral("start"));
}

void DmsController::stopMonitoring()
{
    sendCommand(QStringLiteral("stop"));
}

void DmsController::resetMonitoring()
{
    if (!m_monitoringEnabled)
        return;
    sendCommand(QStringLiteral("reset"));
}

void DmsController::replayCurrentAlert()
{
    if (!m_monitoringEnabled || m_fatigueLevel < 2 || m_message.trimmed().isEmpty())
        return;
    speak(m_message);
    emit alertRequested(m_message, m_fatigueLevel, m_eventId);
}


void DmsController::requestStatus()
{
    if (!m_monitoringEnabled || !m_clientRunning || m_statusReply)
        return;

    QNetworkRequest request(statusUrl());
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI-DMS/1.0"));
    request.setTransferTimeout(kRequestTimeoutMs);

    m_statusReply = m_network->get(request);
    connect(m_statusReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_statusReply;
        m_statusReply = nullptr;
        if (!reply)
            return;
        if (!m_monitoringEnabled) {
            reply->deleteLater();
            return;
        }

        const QByteArray payload = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            m_consecutiveFailures = 0;
            applyStatusPayload(payload);
        } else {
            ++m_consecutiveFailures;
            if (m_consecutiveFailures >= kOfflineFailureThreshold)
                markServiceUnavailable(reply->errorString());
        }
        reply->deleteLater();
    });
}

void DmsController::connectWebSocket()
{
    if (!m_monitoringEnabled || !m_clientRunning)
        return;
    if (m_webSocket.state() == QAbstractSocket::ConnectedState
        || m_webSocket.state() == QAbstractSocket::ConnectingState) {
        return;
    }
    m_webSocket.open(webSocketUrl());
}

void DmsController::handleWebSocketMessage(const QString &message)
{
    if (!m_monitoringEnabled)
        return;
    m_consecutiveFailures = 0;
    applyStatusPayload(message.toUtf8());
}

void DmsController::handleWebSocketConnected()
{
    if (!m_monitoringEnabled) {
        m_webSocket.close();
        return;
    }
    if (!m_serviceAvailable) {
        m_serviceAvailable = true;
        m_lastError.clear();
        emit statusChanged();
    }
}

void DmsController::handleWebSocketDisconnected()
{
    if (m_monitoringEnabled && m_clientRunning && !m_reconnectTimer.isActive())
        m_reconnectTimer.start();
}

void DmsController::loadConfiguration()
{
    QString endpointValue = QStringLiteral("http://127.0.0.1:8765");
    bool voiceValue = true;

    for (const QString &candidate : configurationCandidates()) {
        QFile file(candidate);
        if (!file.open(QIODevice::ReadOnly))
            continue;

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            continue;

        const QJsonObject object = document.object();
        endpointValue = object.value(QStringLiteral("dms_backend_url"))
                            .toString(endpointValue);
        voiceValue = object.value(QStringLiteral("dms_voice_enabled"))
                         .toBool(voiceValue);
        break;
    }

    m_baseUrl = QUrl(normalizedEndpoint(endpointValue));
    m_voiceEnabled = voiceValue;
}

QStringList DmsController::configurationCandidates() const
{
    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();

    auto appendUnique = [&candidates](const QString &path) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (!candidates.contains(absolute))
            candidates.append(absolute);
    };

    appendUnique(QDir(appDir).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(currentDir).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(appDir).filePath(QStringLiteral("../config.json")));
    appendUnique(QDir(appDir).filePath(QStringLiteral("../../config.json")));
    appendUnique(QDir(currentDir).filePath(QStringLiteral("../config.json")));
    return candidates;
}

QUrl DmsController::statusUrl() const
{
    return QUrl(m_baseUrl.toString() + QStringLiteral("/api/v1/dms/status"));
}

QUrl DmsController::commandUrl(const QString &command) const
{
    return QUrl(m_baseUrl.toString()
                + QStringLiteral("/api/v1/dms/")
                + command);
}

QUrl DmsController::webSocketUrl() const
{
    QUrl url(m_baseUrl);
    url.setScheme(url.scheme() == QStringLiteral("https")
                      ? QStringLiteral("wss")
                      : QStringLiteral("ws"));
    url.setPath(QStringLiteral("/api/v1/dms/events"));
    return url;
}

void DmsController::sendCommand(const QString &command, bool applyResponse)
{
    QNetworkRequest request(commandUrl(command));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setTransferTimeout(3000);

    QNetworkReply *reply = m_network->post(request, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, applyResponse]() {
        const QByteArray payload = reply->readAll();
        if (applyResponse && m_monitoringEnabled) {
            if (reply->error() == QNetworkReply::NoError)
                applyStatusPayload(payload);
            else
                markServiceUnavailable(reply->errorString());
        }
        reply->deleteLater();
    });
}

void DmsController::applyStatusPayload(const QByteArray &data)
{
    if (!m_monitoringEnabled)
        return;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;
    applyStatusObject(document.object());
}

void DmsController::applyStatusObject(const QJsonObject &object)
{
    if (!m_monitoringEnabled)
        return;
    const qint64 newBackendStartedAtMs = jsonInteger(object,
                                                     QStringLiteral("started_at_ms"),
                                                     m_backendStartedAtMs);
    const qint64 newEventId = jsonInteger(object,
                                          QStringLiteral("event_id"),
                                          m_eventId);
    const int newFatigueLevel = qBound(0,
                                      object.value(QStringLiteral("fatigue_level"))
                                          .toInt(m_fatigueLevel),
                                      3);
    const QString newMessage = object.value(QStringLiteral("message"))
                                   .toString(m_message);

    m_serviceAvailable = true;
    m_serviceRunning = object.value(QStringLiteral("service_running"))
                           .toBool(m_serviceRunning);
    m_modelsReady = object.value(QStringLiteral("models_ready"))
                        .toBool(m_modelsReady);
    m_cameraAvailable = object.value(QStringLiteral("camera_available"))
                            .toBool(m_cameraAvailable);
    m_faceDetected = object.value(QStringLiteral("face_detected"))
                         .toBool(m_faceDetected);
    m_fatigueLevel = newFatigueLevel;
    m_statusCode = object.value(QStringLiteral("status"))
                       .toString(m_statusCode);
    m_statusText = object.value(QStringLiteral("status_text"))
                       .toString(m_statusText);
    m_monitoringState = object.value(QStringLiteral("monitoring_state"))
                            .toString(m_monitoringState);
    m_lastError = object.value(QStringLiteral("last_error"))
                      .toString();
    m_closedProbability = object.value(QStringLiteral("closed_probability"))
                              .toDouble(m_closedProbability);
    m_closedDurationMs = object.value(QStringLiteral("closed_duration_ms"))
                             .toInt(m_closedDurationMs);
    m_perclos = object.value(QStringLiteral("perclos"))
                    .toDouble(m_perclos);
    m_yawnProbability = object.value(QStringLiteral("yawn_probability"))
                            .toDouble(m_yawnProbability);
    m_yawnCountWindow = object.value(QStringLiteral("yawn_count_window"))
                            .toInt(m_yawnCountWindow);
    m_processedFps = object.value(QStringLiteral("processed_fps"))
                         .toDouble(m_processedFps);
    m_inferenceMs = object.value(QStringLiteral("inference_ms"))
                        .toDouble(m_inferenceMs);
    m_eventId = newEventId;
    m_eventType = object.value(QStringLiteral("event_type"))
                      .toString(m_eventType);
    m_message = newMessage;
    m_consecutiveFailures = 0;

    maybeEmitAlert(newBackendStartedAtMs,
                   newEventId,
                   newFatigueLevel,
                   newMessage);

    if (newBackendStartedAtMs > 0)
        m_backendStartedAtMs = newBackendStartedAtMs;

    emit statusChanged();
}

void DmsController::markServiceUnavailable(const QString &errorText)
{
    if (!m_monitoringEnabled)
        return;
    const bool changed = m_serviceAvailable
                         || m_serviceRunning
                         || m_lastError != errorText;
    m_serviceAvailable = false;
    m_serviceRunning = false;
    m_cameraAvailable = false;
    m_faceDetected = false;
    m_monitoringState = QStringLiteral("offline");
    m_statusText = QStringLiteral("AI 疲劳监测服务离线");
    m_lastError = errorText;
    if (changed)
        emit statusChanged();
}

void DmsController::resetDisabledState()
{
    m_serviceAvailable = false;
    m_serviceRunning = false;
    m_modelsReady = false;
    m_cameraAvailable = false;
    m_faceDetected = false;
    m_fatigueLevel = 1;
    m_statusCode = QStringLiteral("disabled");
    m_statusText = QStringLiteral("疲劳驾驶监测已关闭");
    m_monitoringState = QStringLiteral("disabled");
    m_lastError.clear();
    m_closedProbability = 0.0;
    m_closedDurationMs = 0;
    m_perclos = 0.0;
    m_yawnProbability = 0.0;
    m_yawnCountWindow = 0;
    m_processedFps = 0.0;
    m_inferenceMs = 0.0;
    m_eventType.clear();
    m_message.clear();
    m_consecutiveFailures = 0;
    m_lastHandledEventId = m_eventId;
    emit statusChanged();
}

void DmsController::maybeEmitAlert(qint64 backendStartedAtMs,
                                   qint64 newEventId,
                                   int newFatigueLevel,
                                   const QString &newMessage)
{
    if (!m_monitoringEnabled)
        return;

    if (backendStartedAtMs > 0
        && m_backendStartedAtMs > 0
        && backendStartedAtMs != m_backendStartedAtMs) {
        m_lastHandledEventId = 0;
    }

    if (newEventId <= 0
        || newEventId == m_lastHandledEventId
        || newFatigueLevel < 2
        || newMessage.trimmed().isEmpty()) {
        return;
    }

    m_lastHandledEventId = newEventId;
    speak(newMessage);
    emit alertRequested(newMessage, newFatigueLevel, newEventId);
}

void DmsController::speak(const QString &text)
{
    const QString normalizedText = text.trimmed();
    if (!m_monitoringEnabled || normalizedText.isEmpty() || !m_voiceEnabled)
        return;

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
    if (!m_speech)
        return;

    updateVoiceAvailability();
    if (!m_voiceAvailable) {
        qWarning().noquote() << "[DMS-TTS] 语音引擎不可用:"
                             << m_speech->errorString();
        return;
    }

    configureSpeechVoice();
    if (m_speech->state() == QTextToSpeech::Speaking
        || m_speech->state() == QTextToSpeech::Paused) {
        m_speech->stop();
    }
    m_speech->say(normalizedText);
#endif
}

#ifdef DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH
void DmsController::configureSpeechVoice()
{
    if (!m_speech || m_speechConfigured
        || m_speech->state() == QTextToSpeech::Error) {
        return;
    }

    const QList<QLocale> locales = m_speech->availableLocales();
    QLocale selectedLocale;
    bool foundChinese = false;

    for (const QLocale &locale : locales) {
        if (locale.language() != QLocale::Chinese)
            continue;
        selectedLocale = locale;
        foundChinese = true;
        if (locale.territory() == QLocale::China)
            break;
    }

    if (foundChinese)
        m_speech->setLocale(selectedLocale);

    const QList<QVoice> voices = m_speech->availableVoices();
    if (!voices.isEmpty())
        m_speech->setVoice(voices.first());

    m_speech->setRate(-0.08);
    m_speech->setPitch(0.0);
    m_speech->setVolume(1.0);
    m_speechConfigured = true;
}

void DmsController::updateVoiceAvailability()
{
    const bool available = m_speech
                           && m_speech->state() != QTextToSpeech::Error
                           && m_speech->engineCapabilities().testFlag(
                               QTextToSpeech::Capability::Speak);
    if (m_voiceAvailable == available)
        return;
    m_voiceAvailable = available;
    emit voiceAvailableChanged();
}

#endif

void DmsController::setClientRunning(bool running)
{
    if (m_clientRunning == running)
        return;
    m_clientRunning = running;
    emit clientRunningChanged();
}
