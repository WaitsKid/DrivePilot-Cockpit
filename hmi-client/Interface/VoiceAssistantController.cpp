#include "VoiceAssistantController.h"

#include <QAbstractItemModel>
#include <QAudioDevice>
#include <QAudioSource>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLocale>
#include <QMediaDevices>
#include <QMessageAuthenticationCode>
#include <QSslError>
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>
#include <QWebSocket>
#include <QWebSocketProtocol>
#include <QtEndian>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr int kAudioFrameBytes = 1280;
constexpr int kAudioFrameIntervalMs = 40;
constexpr int kServerEndSilenceMs = 2000;
constexpr int kInitialSpeechTimeoutMs = 7000;
constexpr int kConnectionTimeoutMs = 9000;
constexpr int kFinalResultTimeoutMs = 5000;
constexpr int kMaximumSessionMs = 55000;
constexpr int kVadPollIntervalMs = 100;
constexpr int kAgentConnectionTimeoutMs = 7000;
constexpr int kAgentTaskTimeoutMs = 180000;
constexpr int kAgentReconnectBaseMs = 1500;
constexpr int kAgentReconnectMaxMs = 12000;
constexpr int kTargetSampleRate = 16000;
constexpr qreal kMinimumVoiceThreshold = 0.012;
constexpr int kVoiceFramesRequired = 3;

const QString kWebSocketHost = QStringLiteral("iat-api.xfyun.cn");
const QString kWebSocketPath = QStringLiteral("/v2/iat");

qreal clampAudioLevel(qreal value)
{
    return qBound<qreal>(0.0, value, 1.0);
}

qreal decodeSample(const char *sample, QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8: {
        const auto value = static_cast<quint8>(*sample);
        return (static_cast<int>(value) - 128) / 128.0;
    }
    case QAudioFormat::Int16: {
        qint16 value = 0;
        std::memcpy(&value, sample, sizeof(value));
        return value / 32768.0;
    }
    case QAudioFormat::Int32: {
        qint32 value = 0;
        std::memcpy(&value, sample, sizeof(value));
        return value / 2147483648.0;
    }
    case QAudioFormat::Float: {
        float value = 0.0F;
        std::memcpy(&value, sample, sizeof(value));
        return qBound(-1.0, static_cast<double>(value), 1.0);
    }
    case QAudioFormat::Unknown:
    default:
        return 0.0;
    }
}

bool looksLikeCredentialPlaceholder(const QString &value)
{
    const QString normalized = value.trimmed();
    return normalized.isEmpty()
        || normalized.contains(QStringLiteral("你的"))
        || normalized.compare(QStringLiteral("APPID"), Qt::CaseInsensitive) == 0
        || normalized.compare(QStringLiteral("APIKey"), Qt::CaseInsensitive) == 0
        || normalized.compare(QStringLiteral("APISecret"), Qt::CaseInsensitive) == 0;
}

QStringList speechConfigCandidates()
{
    QStringList candidates;
    const auto appendUnique = [&candidates](const QString &path) {
        const QString cleanPath = QDir::cleanPath(path);
        if (!candidates.contains(cleanPath))
            candidates.append(cleanPath);
    };

    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    const QString workingDirectory = QDir::currentPath();

    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(workingDirectory).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("../config.json")));
    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("../../config.json")));
    appendUnique(QDir(workingDirectory).filePath(QStringLiteral("../config.json")));

    return candidates;
}
}

VoiceAssistantController::VoiceAssistantController(QObject *parent)
    : QObject(parent)
    , m_messages(this)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_agentSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    m_recognitionLanguage = QStringLiteral("普通话 zh_cn · 讯飞流式听写");

    m_frameTimer.setInterval(kAudioFrameIntervalMs);
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_frameTimer, &QTimer::timeout,
            this, &VoiceAssistantController::sendNextAudioFrame);

    m_vadTimer.setInterval(kVadPollIntervalMs);
    connect(&m_vadTimer, &QTimer::timeout, this, [this]() {
        if (!m_sessionActive || !m_listening)
            return;

        if (m_sessionElapsed.isValid() && m_sessionElapsed.elapsed() >= kMaximumSessionMs) {
            setSpeechStatus(QStringLiteral("已达到单次识别时长上限，正在完成识别"));
            finishListening();
            return;
        }

        if (!m_speechDetected) {
            setSilenceRemainingMs(kServerEndSilenceMs);
            if (m_sessionElapsed.isValid()
                && m_sessionElapsed.elapsed() >= kInitialSpeechTimeoutMs) {
                abortRecognition(QStringLiteral("没有检测到有效语音，请靠近麦克风重试"));
            }
            return;
        }

        const int silenceElapsed = m_lastVoiceElapsed.isValid()
            ? static_cast<int>(m_lastVoiceElapsed.elapsed())
            : 0;
        const int remaining = qMax(0, kServerEndSilenceMs - silenceElapsed);
        setSilenceRemainingMs(remaining);
        if (remaining == 0) {
            setSpeechStatus(QStringLiteral("检测到 2 秒静默，正在完成识别"));
            finishListening();
        }
    });

    m_connectionWatchdog.setSingleShot(true);
    m_connectionWatchdog.setInterval(kConnectionTimeoutMs);
    connect(&m_connectionWatchdog, &QTimer::timeout, this, [this]() {
        if (m_initializing)
            abortRecognition(QStringLiteral("连接讯飞语音服务超时，请检查网络与系统时间"));
    });

    m_finalResultWatchdog.setSingleShot(true);
    m_finalResultWatchdog.setInterval(kFinalResultTimeoutMs);
    connect(&m_finalResultWatchdog, &QTimer::timeout, this, [this]() {
        if (!m_finishing || m_resultCommitted)
            return;
        if (!assembledTranscript().trimmed().isEmpty()) {
            completeRecognition();
        } else {
            abortRecognition(QStringLiteral("语音服务未返回有效识别结果"));
        }
    });

    connect(m_socket, &QWebSocket::connected,
            this, &VoiceAssistantController::openSpeechSession);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &VoiceAssistantController::handleSocketMessage);
    connect(m_socket, &QWebSocket::disconnected, this, [this]() {
        m_connectionWatchdog.stop();
        if (m_cancelRequested || m_resultCommitted || !m_sessionActive)
            return;
        if (m_finishing && !assembledTranscript().trimmed().isEmpty()) {
            completeRecognition();
            return;
        }
        abortRecognition(QStringLiteral("讯飞语音连接已断开：%1")
                             .arg(m_socket->errorString()));
    });
    connect(m_socket, &QWebSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
        if (m_cancelRequested || !m_sessionActive)
            return;
        abortRecognition(QStringLiteral("讯飞语音连接失败：%1")
                             .arg(m_socket->errorString()));
    });
    connect(m_socket, &QWebSocket::sslErrors,
            this, [this](const QList<QSslError> &errors) {
        if (errors.isEmpty())
            return;
        abortRecognition(QStringLiteral("讯飞语音 TLS 校验失败：%1")
                             .arg(errors.constFirst().errorString()));
    });

    m_agentReconnectTimer.setSingleShot(true);
    connect(&m_agentReconnectTimer, &QTimer::timeout,
            this, &VoiceAssistantController::connectAgentBackend);

    m_agentConnectionWatchdog.setSingleShot(true);
    m_agentConnectionWatchdog.setInterval(kAgentConnectionTimeoutMs);
    connect(&m_agentConnectionWatchdog, &QTimer::timeout, this, [this]() {
        if (m_agentSocket->state() == QAbstractSocket::ConnectingState) {
            m_agentSocket->abort();
            setAgentStatus(QStringLiteral("连接 Agent 后端超时，准备重试"));
            scheduleAgentReconnect();
        }
    });

    m_agentTaskWatchdog.setSingleShot(true);
    m_agentTaskWatchdog.setInterval(kAgentTaskTimeoutMs);
    connect(&m_agentTaskWatchdog, &QTimer::timeout, this, [this]() {
        if (!m_agentBusy)
            return;
        sendAgentJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("cancel")}});
        setAgentBusy(false);
        setAgentStatus(QStringLiteral("Agent 任务超时"));
        m_messages.appendMessage(
            QStringLiteral("任务执行时间过长，已自动取消。"),
            QStringLiteral("system"),
            QStringLiteral("超时保护"));
        emit agentFailed(QStringLiteral("AI Agent 任务超时"));
    });

    connect(m_agentSocket, &QWebSocket::connected, this, [this]() {
        m_agentConnectionWatchdog.stop();
        m_agentReconnectTimer.stop();
        m_agentReconnectAttempt = 0;
        setAgentConnected(true);
        setAgentStatus(QStringLiteral("Agent 后端已连接，正在等待服务信息"));
    });
    connect(m_agentSocket, &QWebSocket::textMessageReceived,
            this, &VoiceAssistantController::handleAgentSocketMessage);
    connect(m_agentSocket, &QWebSocket::disconnected, this, [this]() {
        m_agentConnectionWatchdog.stop();
        setAgentConnected(false);
        setAgentModelName(QStringLiteral("后端离线"));
        if (m_agentBusy) {
            m_agentTaskWatchdog.stop();
            setAgentBusy(false);
            m_messages.appendMessage(
                QStringLiteral("Agent 后端连接中断，本次任务未能继续。"),
                QStringLiteral("system"),
                QStringLiteral("连接中断"));
        }
        if (m_agentConfigured)
            scheduleAgentReconnect();
    });
    connect(m_agentSocket, &QWebSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
        if (!m_agentConfigured)
            return;
        setAgentStatus(QStringLiteral("Agent 后端连接失败：%1")
                           .arg(m_agentSocket->errorString()));
    });

    loadApiCredentials();
    refreshSpeechAvailability();
    loadAgentConfiguration();
    connectAgentBackend();
    appendWelcomeMessage();
}

VoiceAssistantController::~VoiceAssistantController()
{
    m_cancelRequested = true;
    stopAudioCapture();
    m_agentReconnectTimer.stop();
    m_agentConnectionWatchdog.stop();
    m_agentTaskWatchdog.stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close(QWebSocketProtocol::CloseCodeNormal,
                        QStringLiteral("application shutdown"));
    if (m_agentSocket->state() != QAbstractSocket::UnconnectedState)
        m_agentSocket->close(QWebSocketProtocol::CloseCodeNormal,
                             QStringLiteral("application shutdown"));
}

QAbstractItemModel *VoiceAssistantController::messages()
{
    return &m_messages;
}

QString VoiceAssistantController::draftText() const
{
    return m_draftText;
}

void VoiceAssistantController::setDraftText(const QString &text)
{
    if (m_draftText == text)
        return;
    m_draftText = text;
    emit draftTextChanged();
}

bool VoiceAssistantController::listening() const
{
    return m_listening;
}

bool VoiceAssistantController::initializing() const
{
    return m_initializing;
}

bool VoiceAssistantController::finishing() const
{
    return m_finishing;
}

bool VoiceAssistantController::processing() const
{
    return m_processing;
}

bool VoiceAssistantController::speechSupported() const
{
    return m_speechSupported;
}

bool VoiceAssistantController::apiConfigured() const
{
    return m_apiConfigured;
}

QString VoiceAssistantController::speechStatus() const
{
    return m_speechStatus;
}

QString VoiceAssistantController::recognitionLanguage() const
{
    return m_recognitionLanguage;
}

QString VoiceAssistantController::liveTranscript() const
{
    return m_liveTranscript;
}

qreal VoiceAssistantController::audioLevel() const
{
    return m_audioLevel;
}

bool VoiceAssistantController::speechDetected() const
{
    return m_speechDetected;
}

int VoiceAssistantController::silenceRemainingMs() const
{
    return m_silenceRemainingMs;
}

bool VoiceAssistantController::agentConfigured() const
{
    return m_agentConfigured;
}

bool VoiceAssistantController::agentConnected() const
{
    return m_agentConnected;
}

bool VoiceAssistantController::agentBusy() const
{
    return m_agentBusy;
}

QString VoiceAssistantController::agentStatus() const
{
    return m_agentStatus;
}

QString VoiceAssistantController::agentModelName() const
{
    return m_agentModelName;
}

QString VoiceAssistantController::currentTaskText() const
{
    return m_currentTaskText;
}

void VoiceAssistantController::sendMessage(const QString &text)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty())
        return;

    if (m_agentBusy) {
        emit agentFailed(QStringLiteral("上一项 Agent 任务仍在执行"));
        return;
    }

    if (!m_agentConnected) {
        m_messages.appendMessage(
            normalized,
            QStringLiteral("user"),
            QStringLiteral("发送失败"));
        m_messages.appendMessage(
            QStringLiteral("AI Agent 后端尚未连接。请先启动 Python 服务，再点击“重连服务”。"),
            QStringLiteral("system"),
            QStringLiteral("后端离线"));
        setDraftText(QString());
        connectAgentBackend();
        emit agentFailed(QStringLiteral("AI Agent 后端未连接"));
        return;
    }

    m_messages.appendMessage(normalized,
                             QStringLiteral("user"),
                             QStringLiteral("已发送"));
    setDraftText(QString());
    setAgentBusy(true);
    setAgentStatus(QStringLiteral("正在提交任务"));
    setCurrentTaskText(normalized);
    m_agentTaskWatchdog.start();

    sendAgentJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("user_message")},
        {QStringLiteral("content"), normalized}
    });
}
void VoiceAssistantController::startListening()
{
    if (m_sessionActive || m_initializing || m_listening || m_finishing)
        return;

    loadApiCredentials();
    refreshSpeechAvailability();
    if (!m_apiConfigured) {
        const QString message = QStringLiteral("请先在项目根目录 config.json 中填写讯飞 APPID、APIKey 和 APISecret");
        setSpeechStatus(message);
        emit recognitionFailed(message);
        return;
    }
    if (!m_speechSupported) {
        const QString message = QStringLiteral("没有可用的麦克风输入设备，请检查系统麦克风权限");
        setSpeechStatus(message);
        emit recognitionFailed(message);
        return;
    }

    resetRecognitionSession();
    m_sessionActive = true;
    setInitializing(true);
    setSpeechStatus(QStringLiteral("正在建立讯飞 WebSocket 安全连接"));
    m_connectionWatchdog.start();
    m_socket->open(buildAuthorizedUrl());
}

void VoiceAssistantController::finishListening()
{
    if (!m_sessionActive || m_finishing)
        return;

    setFinishing(true);
    setListening(false);
    setSpeechStatus(QStringLiteral("正在上传剩余音频并生成最终文字"));
    stopAudioCapture();
    m_vadTimer.stop();
    setSilenceRemainingMs(0);

    if (!m_frameTimer.isActive())
        m_frameTimer.start();
}

void VoiceAssistantController::cancelListening()
{
    if (!m_sessionActive && !m_initializing && !m_listening && !m_finishing)
        return;

    m_cancelRequested = true;
    stopAudioCapture();
    m_frameTimer.stop();
    m_vadTimer.stop();
    m_connectionWatchdog.stop();
    m_finalResultWatchdog.stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close(QWebSocketProtocol::CloseCodeNormal,
                        QStringLiteral("user cancelled"));

    m_sessionActive = false;
    setInitializing(false);
    setListening(false);
    setFinishing(false);
    setAudioLevel(0.0);
    setSpeechDetected(false);
    setLiveTranscript(QString());
    setSpeechStatus(QStringLiteral("已取消本次语音输入"));
}

void VoiceAssistantController::stopListening()
{
    cancelListening();
}

void VoiceAssistantController::clearConversation()
{
    if (m_agentBusy)
        cancelAgentTask();
    m_messages.clearMessages();
    if (m_agentConnected)
        sendAgentJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("reset")}});
    appendWelcomeMessage();
}

void VoiceAssistantController::useQuickPrompt(const QString &text)
{
    sendMessage(text);
}

void VoiceAssistantController::appendWelcomeMessage()
{
    m_messages.appendMessage(
        QStringLiteral("你好，我是 DrivePilot 车载助手。你可以输入文字，也可以使用讯飞语音识别。"
                       "涉及车控的指令会通过 Agent 服务转成工具调用，并显示实际执行结果。"),
        QStringLiteral("assistant"),
        QStringLiteral("Agent 已就绪"));
}

void VoiceAssistantController::loadApiCredentials()
{
    QString loadedAppId;
    QString loadedApiKey;
    QString loadedApiSecret;
    QString loadedPath;
    QString lastError;

    const QStringList candidates = speechConfigCandidates();
    for (const QString &candidate : candidates) {
        QFile file(candidate);
        if (!file.exists())
            continue;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lastError = QStringLiteral("无法读取 %1：%2")
                            .arg(QDir::toNativeSeparators(candidate), file.errorString());
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            lastError = QStringLiteral("%1 不是有效的 JSON：%2")
                            .arg(QDir::toNativeSeparators(candidate), parseError.errorString());
            continue;
        }

        const QJsonObject object = document.object();
        const QString appId = object.value(QStringLiteral("xfyun_app_id")).toString().trimmed();
        const QString apiKey = object.value(QStringLiteral("xfyun_api_key")).toString().trimmed();
        const QString apiSecret = object.value(QStringLiteral("xfyun_api_secret")).toString().trimmed();

        if (looksLikeCredentialPlaceholder(appId)
            || looksLikeCredentialPlaceholder(apiKey)
            || looksLikeCredentialPlaceholder(apiSecret)) {
            lastError = QStringLiteral("%1 中仍是示例值，请填写讯飞应用凭据")
                            .arg(QDir::toNativeSeparators(candidate));
            continue;
        }

        loadedAppId = appId;
        loadedApiKey = apiKey;
        loadedApiSecret = apiSecret;
        loadedPath = QFileInfo(candidate).absoluteFilePath();
        break;
    }

    const bool configured = !loadedAppId.isEmpty()
        && !loadedApiKey.isEmpty()
        && !loadedApiSecret.isEmpty();
    const bool configuredChanged = configured != m_apiConfigured;

    m_appId = loadedAppId;
    m_apiKey = loadedApiKey;
    m_apiSecret = loadedApiSecret;
    m_apiConfigured = configured;

    if (configuredChanged)
        emit apiConfiguredChanged();

    if (configured) {
        qInfo() << "XFYUN config loaded from:"
                << QDir::toNativeSeparators(loadedPath);
    } else if (!lastError.isEmpty()) {
        qWarning().noquote() << lastError;
    } else {
        qWarning() << "XFYUN config.json was not found. Checked:" << candidates;
    }
}

void VoiceAssistantController::refreshSpeechAvailability()
{
    const QAudioDevice input = QMediaDevices::defaultAudioInput();
    setSpeechSupported(m_apiConfigured && !input.isNull());

    if (!m_apiConfigured) {
        setSpeechStatus(QStringLiteral("未配置讯飞 WebAPI，请填写项目根目录 config.json"));
    } else if (input.isNull()) {
        setSpeechStatus(QStringLiteral("未检测到麦克风，请检查系统权限与输入设备"));
    } else if (!m_sessionActive) {
        setSpeechStatus(QStringLiteral("讯飞流式语音识别就绪"));
    }
}

void VoiceAssistantController::setListening(bool value)
{
    if (m_listening == value)
        return;
    m_listening = value;
    emit listeningChanged();
}

void VoiceAssistantController::setInitializing(bool value)
{
    if (m_initializing == value)
        return;
    m_initializing = value;
    emit initializingChanged();
}

void VoiceAssistantController::setFinishing(bool value)
{
    if (m_finishing == value)
        return;
    m_finishing = value;
    emit finishingChanged();
}

void VoiceAssistantController::setProcessing(bool value)
{
    if (m_processing == value)
        return;
    m_processing = value;
    emit processingChanged();
}

void VoiceAssistantController::setSpeechSupported(bool value)
{
    if (m_speechSupported == value)
        return;
    m_speechSupported = value;
    emit speechSupportedChanged();
}

void VoiceAssistantController::setSpeechStatus(const QString &status)
{
    if (m_speechStatus == status)
        return;
    m_speechStatus = status;
    emit speechStatusChanged();
}

void VoiceAssistantController::setRecognitionLanguage(const QString &language)
{
    if (m_recognitionLanguage == language)
        return;
    m_recognitionLanguage = language;
    emit recognitionLanguageChanged();
}

void VoiceAssistantController::setLiveTranscript(const QString &text)
{
    if (m_liveTranscript == text)
        return;
    m_liveTranscript = text;
    emit liveTranscriptChanged();
}

void VoiceAssistantController::setAudioLevel(qreal level)
{
    const qreal normalized = clampAudioLevel(level);
    if (qAbs(m_audioLevel - normalized) < 0.01)
        return;
    m_audioLevel = normalized;
    emit audioLevelChanged();
}

void VoiceAssistantController::setSpeechDetected(bool value)
{
    if (m_speechDetected == value)
        return;
    m_speechDetected = value;
    emit speechDetectedChanged();
}

void VoiceAssistantController::setSilenceRemainingMs(int value)
{
    const int normalized = qMax(0, value);
    if (m_silenceRemainingMs == normalized)
        return;
    m_silenceRemainingMs = normalized;
    emit silenceRemainingMsChanged();
}

void VoiceAssistantController::reloadAgentConfiguration()
{
    if (m_agentBusy)
        cancelAgentTask();
    loadAgentConfiguration();
    reconnectAgentBackend();
}

void VoiceAssistantController::reconnectAgentBackend()
{
    m_agentReconnectTimer.stop();
    m_agentConnectionWatchdog.stop();
    if (m_agentSocket->state() != QAbstractSocket::UnconnectedState)
        m_agentSocket->abort();
    setAgentConnected(false);
    m_agentReconnectAttempt = 0;
    connectAgentBackend();
}

void VoiceAssistantController::cancelAgentTask()
{
    if (!m_agentBusy)
        return;
    sendAgentJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("cancel")}});
    m_agentTaskWatchdog.stop();
    setAgentBusy(false);
    setAgentStatus(QStringLiteral("任务已取消"));
    setCurrentTaskText(QString());
}

void VoiceAssistantController::submitToolResult(const QString &callId,
                                                const QString &toolName,
                                                bool success,
                                                const QString &message,
                                                const QVariantMap &data)
{
    if (!m_agentConnected || callId.trimmed().isEmpty())
        return;

    sendAgentJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("tool_result")},
        {QStringLiteral("call_id"), callId},
        {QStringLiteral("name"), toolName},
        {QStringLiteral("success"), success},
        {QStringLiteral("message"), message},
        {QStringLiteral("data"), QJsonObject::fromVariantMap(data)}
    });
}

void VoiceAssistantController::loadAgentConfiguration()
{
    QString backendUrl = QStringLiteral("ws://127.0.0.1:8770/ws/agent");
    QString loadedPath;

    for (const QString &candidate : speechConfigCandidates()) {
        QFile file(candidate);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            continue;

        const QString configured = document.object()
                                       .value(QStringLiteral("ai_agent_backend_url"))
                                       .toString()
                                       .trimmed();
        if (!configured.isEmpty())
            backendUrl = configured;
        loadedPath = QFileInfo(candidate).absoluteFilePath();
        break;
    }

    const bool configured = QUrl(backendUrl).isValid();
    const bool configuredChanged = configured != m_agentConfigured;
    m_agentBackendUrl = backendUrl;
    m_agentConfigured = configured;
    if (configuredChanged)
        emit agentConfiguredChanged();

    setAgentModelName(QStringLiteral("后端未连接"));
    setAgentStatus(configured
                       ? QStringLiteral("等待连接 Python Agent 后端")
                       : QStringLiteral("Agent 后端地址无效"));

    if (!loadedPath.isEmpty()) {
        qInfo() << "AI Agent config loaded from:"
                << QDir::toNativeSeparators(loadedPath)
                << "backend=" << m_agentBackendUrl;
    }
}

QUrl VoiceAssistantController::agentWebSocketUrl() const
{
    QUrl url(m_agentBackendUrl);
    if (url.scheme() == QStringLiteral("http"))
        url.setScheme(QStringLiteral("ws"));
    else if (url.scheme() == QStringLiteral("https"))
        url.setScheme(QStringLiteral("wss"));

    QString path = url.path();
    while (path.endsWith(QLatin1Char('/')))
        path.chop(1);
    if (!path.endsWith(QStringLiteral("/ws/agent"))) {
        if (path.isEmpty())
            path = QStringLiteral("/ws/agent");
        else if (!path.contains(QStringLiteral("/ws/agent")))
            path += QStringLiteral("/ws/agent");
    }
    path += QLatin1Char('/') + m_agentSessionId;
    url.setPath(path);
    return url;
}

void VoiceAssistantController::connectAgentBackend()
{
    if (!m_agentConfigured)
        return;
    if (m_agentSocket->state() == QAbstractSocket::ConnectedState
        || m_agentSocket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    if (m_agentSessionId.isEmpty())
        m_agentSessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QUrl url = agentWebSocketUrl();
    setAgentStatus(QStringLiteral("正在连接 Python Agent 后端"));
    m_agentConnectionWatchdog.start();
    m_agentSocket->open(url);
}

void VoiceAssistantController::scheduleAgentReconnect()
{
    if (!m_agentConfigured || m_agentReconnectTimer.isActive())
        return;

    const int exponent = qMin(m_agentReconnectAttempt, 3);
    const int interval = qMin(kAgentReconnectMaxMs,
                              kAgentReconnectBaseMs * (1 << exponent));
    ++m_agentReconnectAttempt;
    setAgentStatus(QStringLiteral("Agent 后端离线，%1 秒后自动重连")
                       .arg(interval / 1000.0, 0, 'f', 1));
    m_agentReconnectTimer.start(interval);
}

void VoiceAssistantController::sendAgentJson(const QJsonObject &object)
{
    if (m_agentSocket->state() != QAbstractSocket::ConnectedState)
        return;
    m_agentSocket->sendTextMessage(
        QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

void VoiceAssistantController::handleAgentSocketMessage(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "Invalid Agent WebSocket message:" << parseError.errorString();
        return;
    }

    const QJsonObject object = document.object();
    const QString type = object.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("connected")) {
        setAgentConnected(true);
        const QString model = object.value(QStringLiteral("model")).toString();
        setAgentModelName(model.isEmpty() ? QStringLiteral("Agent 后端") : model);
        setAgentStatus(object.value(QStringLiteral("model_configured")).toBool()
                           ? QStringLiteral("云端 Agent 在线")
                           : QStringLiteral("本地规则服务在线"));
        return;
    }

    if (type == QStringLiteral("analysis")) {
        const QString content = object.value(QStringLiteral("content")).toString().trimmed();
        if (!content.isEmpty())
            m_messages.appendMessage(content, QStringLiteral("agent"), QStringLiteral("处理状态"));
        setAgentStatus(QStringLiteral("正在处理"));
        setCurrentTaskText(content);
        return;
    }

    if (type == QStringLiteral("plan")) {
        const QString content = object.value(QStringLiteral("content")).toString().trimmed();
        if (!content.isEmpty())
            m_messages.appendMessage(content, QStringLiteral("agent"), QStringLiteral("待执行操作"));
        setAgentStatus(QStringLiteral("准备执行"));
        setCurrentTaskText(content);
        return;
    }

    if (type == QStringLiteral("tool_call")) {
        const QString callId = object.value(QStringLiteral("call_id")).toString();
        const QString toolName = object.value(QStringLiteral("name")).toString();
        const QString display = object.value(QStringLiteral("display")).toString();
        const QVariantMap arguments = object.value(QStringLiteral("arguments"))
                                          .toObject()
                                          .toVariantMap();
        m_messages.appendMessage(
            display.isEmpty() ? QStringLiteral("正在调用工具：%1").arg(toolName) : display,
            QStringLiteral("tool"),
            QStringLiteral("执行中"));
        setAgentStatus(QStringLiteral("正在执行车机操作"));
        setCurrentTaskText(display);
        emit toolActionRequested(callId, toolName, arguments);
        return;
    }

    if (type == QStringLiteral("observation")) {
        const bool success = object.value(QStringLiteral("success")).toBool();
        const QString content = object.value(QStringLiteral("content")).toString().trimmed();
        m_messages.appendMessage(
            content.isEmpty() ? QStringLiteral("工具已返回执行结果") : content,
            QStringLiteral("tool"),
            success ? QStringLiteral("执行成功") : QStringLiteral("执行失败"));
        return;
    }

    if (type == QStringLiteral("final")) {
        const QString content = object.value(QStringLiteral("content")).toString().trimmed();
        if (!content.isEmpty())
            m_messages.appendMessage(content, QStringLiteral("assistant"), QStringLiteral("回复"));
        setAgentStatus(QStringLiteral("任务完成"));
        setCurrentTaskText(QString());
        return;
    }

    if (type == QStringLiteral("error")) {
        const QString errorMessage = object.value(QStringLiteral("message"))
                                         .toString(QStringLiteral("Agent 后端发生未知错误"));
        m_messages.appendMessage(errorMessage,
                                 QStringLiteral("system"),
                                 QStringLiteral("Agent 错误"));
        m_agentTaskWatchdog.stop();
        setAgentBusy(false);
        setAgentStatus(QStringLiteral("任务失败"));
        emit agentFailed(errorMessage);
        return;
    }

    if (type == QStringLiteral("cancelled")) {
        const QString content = object.value(QStringLiteral("message"))
                                    .toString(QStringLiteral("当前任务已取消。"));
        m_messages.appendMessage(content, QStringLiteral("system"), QStringLiteral("已取消"));
        setAgentStatus(QStringLiteral("任务已取消"));
        return;
    }

    if (type == QStringLiteral("session_reset")) {
        setAgentStatus(QStringLiteral("会话已重置"));
        return;
    }

    if (type == QStringLiteral("done")) {
        m_agentTaskWatchdog.stop();
        setAgentBusy(false);
        if (m_agentStatus != QStringLiteral("任务失败")
            && m_agentStatus != QStringLiteral("任务已取消")) {
            setAgentStatus(QStringLiteral("Agent 在线"));
        }
        setCurrentTaskText(QString());
    }
}

void VoiceAssistantController::setAgentConfigured(bool value)
{
    if (m_agentConfigured == value)
        return;
    m_agentConfigured = value;
    emit agentConfiguredChanged();
}

void VoiceAssistantController::setAgentConnected(bool value)
{
    if (m_agentConnected == value)
        return;
    m_agentConnected = value;
    emit agentConnectedChanged();
}

void VoiceAssistantController::setAgentBusy(bool value)
{
    if (m_agentBusy == value)
        return;
    m_agentBusy = value;
    setProcessing(value);
    emit agentBusyChanged();
}

void VoiceAssistantController::setAgentStatus(const QString &status)
{
    if (m_agentStatus == status)
        return;
    m_agentStatus = status;
    emit agentStatusChanged();
}

void VoiceAssistantController::setAgentModelName(const QString &name)
{
    if (m_agentModelName == name)
        return;
    m_agentModelName = name;
    emit agentModelNameChanged();
}

void VoiceAssistantController::setCurrentTaskText(const QString &text)
{
    if (m_currentTaskText == text)
        return;
    m_currentTaskText = text;
    emit currentTaskTextChanged();
}

QUrl VoiceAssistantController::buildAuthorizedUrl() const
{
    const QString date = QLocale::c().toString(
        QDateTime::currentDateTimeUtc(),
        QStringLiteral("ddd, dd MMM yyyy HH:mm:ss 'GMT'"));
    const QString signatureOrigin = QStringLiteral(
        "host: %1\ndate: %2\nGET %3 HTTP/1.1")
                                        .arg(kWebSocketHost, date, kWebSocketPath);

    QMessageAuthenticationCode authenticationCode(
        QCryptographicHash::Sha256,
        m_apiSecret.toUtf8());
    authenticationCode.addData(signatureOrigin.toUtf8());
    const QString signature = QString::fromLatin1(
        authenticationCode.result().toBase64());

    const QString authorizationOrigin = QStringLiteral(
        "api_key=\"%1\", algorithm=\"hmac-sha256\", "
        "headers=\"host date request-line\", signature=\"%2\"")
                                            .arg(m_apiKey, signature);
    const QString authorization = QString::fromLatin1(
        authorizationOrigin.toUtf8().toBase64());

    QUrl url;
    url.setScheme(QStringLiteral("wss"));
    url.setHost(kWebSocketHost);
    url.setPath(kWebSocketPath);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("authorization"), authorization);
    query.addQueryItem(QStringLiteral("date"), date);
    query.addQueryItem(QStringLiteral("host"), kWebSocketHost);
    url.setQuery(query);
    return url;
}

void VoiceAssistantController::openSpeechSession()
{
    if (!m_sessionActive || m_cancelRequested)
        return;

    m_connectionWatchdog.stop();
    QString errorMessage;
    if (!startAudioCapture(&errorMessage)) {
        abortRecognition(errorMessage);
        return;
    }

    setInitializing(false);
    setListening(true);
    setSpeechStatus(QStringLiteral("正在聆听，请自然说话"));
    m_sessionElapsed.start();
    m_frameTimer.start();
    m_vadTimer.start();
}

bool VoiceAssistantController::startAudioCapture(QString *errorMessage)
{
    const QAudioDevice input = QMediaDevices::defaultAudioInput();
    if (input.isNull()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("没有可用的麦克风输入设备");
        return false;
    }

    QAudioFormat requestedFormat;
    requestedFormat.setSampleRate(kTargetSampleRate);
    requestedFormat.setChannelCount(1);
    requestedFormat.setSampleFormat(QAudioFormat::Int16);

    m_captureFormat = input.isFormatSupported(requestedFormat)
        ? requestedFormat
        : input.preferredFormat();

    if (!m_captureFormat.isValid()
        || m_captureFormat.sampleRate() <= 0
        || m_captureFormat.channelCount() <= 0
        || m_captureFormat.sampleFormat() == QAudioFormat::Unknown) {
        if (errorMessage)
            *errorMessage = QStringLiteral("麦克风没有可用的 PCM 采集格式");
        return false;
    }

    m_audioSource = new QAudioSource(input, m_captureFormat, this);
    connect(m_audioSource, &QAudioSource::stateChanged,
            this, [this](QtAudio::State state) {
        if (state != QtAudio::StoppedState || !m_audioSource)
            return;
        if (m_audioSource->error() != QtAudio::NoError
            && m_sessionActive
            && !m_cancelRequested
            && !m_finishing) {
            abortRecognition(QStringLiteral("麦克风采集失败，错误码：%1")
                                 .arg(static_cast<int>(m_audioSource->error())));
        }
    });

    m_audioDevice = m_audioSource->start();
    if (!m_audioDevice || m_audioSource->error() != QtAudio::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法启动麦克风采集，错误码：%1")
                                .arg(static_cast<int>(m_audioSource->error()));
        }
        stopAudioCapture();
        return false;
    }

    connect(m_audioDevice, &QIODevice::readyRead,
            this, &VoiceAssistantController::readCapturedAudio);

    if (m_captureFormat != requestedFormat) {
        setSpeechStatus(QStringLiteral("麦克风已启动，正在实时重采样为 16kHz 单声道"));
    }
    return true;
}

void VoiceAssistantController::stopAudioCapture()
{
    if (m_audioDevice)
        disconnect(m_audioDevice, nullptr, this, nullptr);
    m_audioDevice = nullptr;

    if (m_audioSource) {
        m_audioSource->stop();
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
    }
}

void VoiceAssistantController::resetRecognitionSession()
{
    m_cancelRequested = false;
    m_finalPacketSent = false;
    m_firstAudioPacketSent = false;
    m_resultCommitted = false;
    m_noiseFloor = 0.008;
    m_consecutiveVoiceFrames = 0;
    m_captureRemainder.clear();
    m_pcmBuffer.clear();
    m_resampleBuffer.clear();
    m_resamplePosition = 0.0;
    m_resultSegments.clear();
    m_sessionElapsed.invalidate();
    m_lastVoiceElapsed.invalidate();
    setInitializing(false);
    setListening(false);
    setFinishing(false);
    setLiveTranscript(QString());
    setAudioLevel(0.0);
    setSpeechDetected(false);
    setSilenceRemainingMs(kServerEndSilenceMs);
}

void VoiceAssistantController::abortRecognition(const QString &message, bool notifyUser)
{
    const bool wasActive = m_sessionActive || m_initializing || m_listening || m_finishing;
    m_cancelRequested = true;
    m_sessionActive = false;
    stopAudioCapture();
    m_frameTimer.stop();
    m_vadTimer.stop();
    m_connectionWatchdog.stop();
    m_finalResultWatchdog.stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close(QWebSocketProtocol::CloseCodeNormal,
                        QStringLiteral("session aborted"));

    setInitializing(false);
    setListening(false);
    setFinishing(false);
    setAudioLevel(0.0);
    setSpeechStatus(message);
    if (notifyUser && wasActive)
        emit recognitionFailed(message);
}

void VoiceAssistantController::completeRecognition()
{
    if (m_resultCommitted)
        return;

    const QString text = assembledTranscript().trimmed();
    if (text.isEmpty()) {
        abortRecognition(QStringLiteral("没有识别到清晰语音，请重试"));
        return;
    }

    m_resultCommitted = true;
    m_sessionActive = false;
    stopAudioCapture();
    m_frameTimer.stop();
    m_vadTimer.stop();
    m_connectionWatchdog.stop();
    m_finalResultWatchdog.stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close(QWebSocketProtocol::CloseCodeNormal,
                        QStringLiteral("recognition completed"));

    setInitializing(false);
    setListening(false);
    setFinishing(false);
    setAudioLevel(0.0);
    setLiveTranscript(text);
    setSpeechStatus(QStringLiteral("识别完成，消息已发送"));
    emit recognitionCompleted(text);
    sendMessage(text);
}

void VoiceAssistantController::readCapturedAudio()
{
    if (!m_audioDevice || !m_sessionActive || m_finishing)
        return;
    const QByteArray data = m_audioDevice->readAll();
    if (!data.isEmpty())
        convertCapturedAudio(data);
}

void VoiceAssistantController::convertCapturedAudio(const QByteArray &rawData)
{
    m_captureRemainder.append(rawData);
    const int bytesPerSample = m_captureFormat.bytesPerSample();
    const int channelCount = m_captureFormat.channelCount();
    const int bytesPerFrame = bytesPerSample * channelCount;
    if (bytesPerSample <= 0 || bytesPerFrame <= 0)
        return;

    const int completeBytes = (m_captureRemainder.size() / bytesPerFrame) * bytesPerFrame;
    if (completeBytes <= 0)
        return;

    const QByteArray completeData = m_captureRemainder.left(completeBytes);
    m_captureRemainder.remove(0, completeBytes);
    const int frameCount = completeData.size() / bytesPerFrame;
    m_resampleBuffer.reserve(m_resampleBuffer.size() + frameCount);

    const char *raw = completeData.constData();
    for (int frame = 0; frame < frameCount; ++frame) {
        qreal mono = 0.0;
        const char *frameStart = raw + frame * bytesPerFrame;
        for (int channel = 0; channel < channelCount; ++channel) {
            mono += decodeSample(frameStart + channel * bytesPerSample,
                                 m_captureFormat.sampleFormat());
        }
        mono /= channelCount;
        m_resampleBuffer.append(static_cast<float>(qBound(-1.0, mono, 1.0)));
    }

    const double sourceStep = static_cast<double>(m_captureFormat.sampleRate())
        / kTargetSampleRate;
    QByteArray converted;
    converted.reserve(static_cast<int>(m_resampleBuffer.size() / sourceStep) * 2);

    while (m_resamplePosition + 1.0 < m_resampleBuffer.size()) {
        const int leftIndex = static_cast<int>(std::floor(m_resamplePosition));
        const int rightIndex = leftIndex + 1;
        const double fraction = m_resamplePosition - leftIndex;
        const double sample = m_resampleBuffer.at(leftIndex) * (1.0 - fraction)
            + m_resampleBuffer.at(rightIndex) * fraction;
        const qint16 pcmSample = static_cast<qint16>(
            qBound(-32768.0, sample * 32767.0, 32767.0));
        converted.append(reinterpret_cast<const char *>(&pcmSample), sizeof(pcmSample));
        m_resamplePosition += sourceStep;
    }

    const int consumedSamples = static_cast<int>(std::floor(m_resamplePosition));
    if (consumedSamples > 0) {
        m_resampleBuffer.remove(0, qMin(consumedSamples, m_resampleBuffer.size()));
        m_resamplePosition -= consumedSamples;
    }

    if (!converted.isEmpty())
        appendPcm16Samples(converted);
}

void VoiceAssistantController::appendPcm16Samples(const QByteArray &pcmData)
{
    m_pcmBuffer.append(pcmData);
    analyzeVoiceActivity(pcmData);
}

void VoiceAssistantController::analyzeVoiceActivity(const QByteArray &pcmData)
{
    const int sampleCount = pcmData.size() / static_cast<int>(sizeof(qint16));
    if (sampleCount <= 0)
        return;

    const qint16 *samples = reinterpret_cast<const qint16 *>(pcmData.constData());
    long double squareSum = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        const long double normalized = samples[i] / 32768.0L;
        squareSum += normalized * normalized;
    }
    const qreal rms = static_cast<qreal>(std::sqrt(squareSum / sampleCount));
    setAudioLevel(qMin<qreal>(1.0, rms * 6.5));

    if (!m_speechDetected
        && m_sessionElapsed.isValid()
        && m_sessionElapsed.elapsed() < 600) {
        m_noiseFloor = m_noiseFloor * 0.92 + rms * 0.08;
    }

    const qreal threshold = qMax(kMinimumVoiceThreshold, m_noiseFloor * 3.0);
    if (rms >= threshold) {
        ++m_consecutiveVoiceFrames;
        m_lastVoiceElapsed.restart();
        if (!m_speechDetected && m_consecutiveVoiceFrames >= kVoiceFramesRequired) {
            setSpeechDetected(true);
            setSpeechStatus(QStringLiteral("已检测到语音，停顿 2 秒将自动完成"));
        }
    } else {
        m_consecutiveVoiceFrames = 0;
    }
}

void VoiceAssistantController::sendNextAudioFrame()
{
    if (!m_sessionActive
        || m_socket->state() != QAbstractSocket::ConnectedState)
        return;

    if (m_pcmBuffer.size() >= kAudioFrameBytes) {
        const QByteArray frame = m_pcmBuffer.left(kAudioFrameBytes);
        m_pcmBuffer.remove(0, kAudioFrameBytes);
        const int status = m_firstAudioPacketSent ? 1 : 0;
        sendAudioPacket(status, frame, !m_firstAudioPacketSent);
        m_firstAudioPacketSent = true;
        return;
    }

    if (!m_finishing)
        return;

    if (!m_pcmBuffer.isEmpty()) {
        const QByteArray remainder = m_pcmBuffer;
        m_pcmBuffer.clear();
        const int status = m_firstAudioPacketSent ? 1 : 0;
        sendAudioPacket(status, remainder, !m_firstAudioPacketSent);
        m_firstAudioPacketSent = true;
        return;
    }

    sendFinalPacket();
}

void VoiceAssistantController::sendAudioPacket(int status,
                                               const QByteArray &audioData,
                                               bool includeSessionParameters)
{
    QJsonObject root;
    if (includeSessionParameters) {
        root.insert(QStringLiteral("common"),
                    QJsonObject{{QStringLiteral("app_id"), m_appId}});
        root.insert(QStringLiteral("business"),
                    QJsonObject{
                        {QStringLiteral("language"), QStringLiteral("zh_cn")},
                        {QStringLiteral("domain"), QStringLiteral("iat")},
                        {QStringLiteral("accent"), QStringLiteral("mandarin")},
                        {QStringLiteral("eos"), kServerEndSilenceMs},
                        {QStringLiteral("dwa"), QStringLiteral("wpgs")},
                        {QStringLiteral("ptt"), 1},
                        {QStringLiteral("rlang"), QStringLiteral("zh-cn")},
                        {QStringLiteral("nunum"), 1}
                    });
    }

    root.insert(QStringLiteral("data"),
                QJsonObject{
                    {QStringLiteral("status"), status},
                    {QStringLiteral("format"), QStringLiteral("audio/L16;rate=16000")},
                    {QStringLiteral("encoding"), QStringLiteral("raw")},
                    {QStringLiteral("audio"), QString::fromLatin1(audioData.toBase64())}
                });
    m_socket->sendTextMessage(QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void VoiceAssistantController::sendFinalPacket()
{
    if (m_finalPacketSent)
        return;

    if (!m_firstAudioPacketSent) {
        abortRecognition(QStringLiteral("没有采集到有效音频数据"));
        return;
    }

    m_finalPacketSent = true;
    m_frameTimer.stop();
    QJsonObject root;
    root.insert(QStringLiteral("data"),
                QJsonObject{{QStringLiteral("status"), 2}});
    m_socket->sendTextMessage(QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact)));
    m_finalResultWatchdog.start();
}

void VoiceAssistantController::handleSocketMessage(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        abortRecognition(QStringLiteral("讯飞返回了无法解析的数据"));
        return;
    }

    const QJsonObject root = document.object();
    const int code = root.value(QStringLiteral("code")).toInt(-1);
    if (code != 0) {
        const QString detail = root.value(QStringLiteral("message")).toString();
        abortRecognition(QStringLiteral("讯飞语音错误 %1：%2").arg(code).arg(detail));
        return;
    }

    const QJsonObject dataObject = root.value(QStringLiteral("data")).toObject();
    const QJsonObject resultObject = dataObject.value(QStringLiteral("result")).toObject();
    if (!resultObject.isEmpty())
        applyRecognitionResult(resultObject);

    if (dataObject.value(QStringLiteral("status")).toInt() == 2)
        completeRecognition();
}

void VoiceAssistantController::applyRecognitionResult(const QJsonObject &resultObject)
{
    const int sequence = resultObject.value(QStringLiteral("sn")).toInt();
    const QString correctionType = resultObject.value(QStringLiteral("pgs")).toString();
    if (correctionType == QStringLiteral("rpl")) {
        const QJsonArray range = resultObject.value(QStringLiteral("rg")).toArray();
        if (range.size() >= 2) {
            const int first = range.at(0).toInt();
            const int last = range.at(1).toInt();
            for (int index = first; index <= last; ++index)
                m_resultSegments.remove(index);
        }
    }

    const QString text = extractResultText(resultObject);
    if (!text.isEmpty())
        m_resultSegments.insert(sequence, text);

    setLiveTranscript(assembledTranscript());
}

QString VoiceAssistantController::extractResultText(const QJsonObject &resultObject) const
{
    QString text;
    const QJsonArray wordSegments = resultObject.value(QStringLiteral("ws")).toArray();
    for (const QJsonValue &segmentValue : wordSegments) {
        const QJsonArray candidates = segmentValue.toObject()
                                          .value(QStringLiteral("cw"))
                                          .toArray();
        if (!candidates.isEmpty())
            text.append(candidates.at(0).toObject()
                            .value(QStringLiteral("w"))
                            .toString());
    }
    return text;
}

QString VoiceAssistantController::assembledTranscript() const
{
    QString text;
    for (auto iterator = m_resultSegments.cbegin();
         iterator != m_resultSegments.cend();
         ++iterator) {
        text.append(iterator.value());
    }
    return text;
}
