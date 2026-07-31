#include "VideoController.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>
#include <QtConcurrent>
#include <QtGlobal>

namespace {
constexpr qint64 kResumeThresholdMs = 1500;
constexpr qint64 kNearEndThresholdMs = 1200;
constexpr qint64 kProgressWriteStepMs = 4000;
}

VideoController::VideoController(QObject *parent)
    : QAbstractListModel(parent)
    , m_videos({
          {QStringLiteral("urban-pulse"),
           QStringLiteral("Urban Pulse"),
           QStringLiteral("城市光影与道路节奏"),
           QStringLiteral("城市"),
           QStringLiteral("00:08"),
           QStringLiteral("urban_pulse.mp4"),
           QStringLiteral("urban_pulse.jpg"),
           QStringLiteral("#5C7CFF")},
          {QStringLiteral("coastal-drive"),
           QStringLiteral("Coastal Drive"),
           QStringLiteral("海岸色彩与流体动效"),
           QStringLiteral("旅行"),
           QStringLiteral("00:08"),
           QStringLiteral("coastal_drive.mp4"),
           QStringLiteral("coastal_drive.jpg"),
           QStringLiteral("#21B8C7")},
          {QStringLiteral("night-geometry"),
           QStringLiteral("Night Geometry"),
           QStringLiteral("夜间几何与实时视觉序列"),
           QStringLiteral("视觉"),
           QStringLiteral("00:08"),
           QStringLiteral("night_geometry.mp4"),
           QStringLiteral("night_geometry.jpg"),
           QStringLiteral("#B96CFF")}
      })
{
    m_player.setAudioOutput(&m_audioOutput);

    loadImportedVideos();
    loadSettings();
    m_audioOutput.setVolume(static_cast<float>(m_volume) / 10.0F);

    connect(&m_player,
            &QMediaPlayer::playingChanged,
            this,
            [this](bool) { emit playingChanged(); });
    connect(&m_player,
            &QMediaPlayer::positionChanged,
            this,
            [this](qint64 currentPosition) {
                emit positionChanged(currentPosition);

                if (m_currentIndex < 0 || m_currentIndex >= m_videos.size())
                    return;

                m_resumePositions.insert(currentId(), currentPosition);
                const int progressPercent = duration() > 0
                    ? qBound(0, qRound(currentPosition * 100.0 / duration()), 100)
                    : 0;
                if (progressPercent != m_lastProgressPercent) {
                    m_lastProgressPercent = progressPercent;
                    emitRowChanged(m_currentIndex, {ResumePositionRole, ResumeProgressRole});
                }
            });
    connect(&m_player,
            &QMediaPlayer::durationChanged,
            this,
            [this](qint64 value) {
                emit durationChanged(value);
                if (value > 0 && m_currentIndex >= 0 && m_currentIndex < m_videos.size())
                    m_knownDurations.insert(currentId(), value);
                emitRowChanged(m_currentIndex, {DurationTextRole, ResumeProgressRole});
            });
    connect(&m_player,
            &QMediaPlayer::seekableChanged,
            this,
            [this](bool) { emit seekableChanged(); });
    connect(&m_player,
            &QMediaPlayer::errorOccurred,
            this,
            [this](QMediaPlayer::Error, const QString &message) {
                const QString text = message.trimmed().isEmpty()
                    ? QStringLiteral("视频播放失败，请检查媒体文件或 Qt Multimedia 后端")
                    : message.trimmed();
                m_pendingPlay = false;
                setLoading(false);
                setSourceReady(false);
                setErrorString(text);
                emit playbackError(text);
            });
    connect(&m_player,
            &QMediaPlayer::mediaStatusChanged,
            this,
            [this](QMediaPlayer::MediaStatus status) {
                switch (status) {
                case QMediaPlayer::LoadingMedia:
                case QMediaPlayer::StalledMedia:
                    setLoading(true);
                    setSourceReady(false);
                    break;
                case QMediaPlayer::LoadedMedia:
                case QMediaPlayer::BufferedMedia: {
                    setLoading(false);
                    setSourceReady(true);
                    const qint64 requestedPosition = m_pendingResumePosition;
                    m_pendingResumePosition = 0;
                    if (requestedPosition > kResumeThresholdMs
                        && (duration() <= 0 || requestedPosition < duration() - kNearEndThresholdMs)) {
                        m_player.setPosition(requestedPosition);
                    }
                    if (m_pendingPlay) {
                        m_pendingPlay = false;
                        m_player.play();
                    }
                    break;
                }
                case QMediaPlayer::EndOfMedia:
                    m_resumePositions.insert(currentId(), 0);
                    m_lastPersistedPosition = -1;
                    emitRowChanged(m_currentIndex, {ResumePositionRole, ResumeProgressRole});
                    saveSettings();
                    if (m_autoPlayNext && !m_videos.isEmpty())
                        loadVideo((m_currentIndex + 1) % m_videos.size(), true, true);
                    else
                        m_player.setPosition(0);
                    break;
                case QMediaPlayer::InvalidMedia:
                    m_pendingPlay = false;
                    setLoading(false);
                    setSourceReady(false);
                    if (m_errorString.isEmpty()) {
                        setErrorString(QStringLiteral("当前视频无法解码，请确认系统支持 H.264/AAC"));
                        emit playbackError(m_errorString);
                    }
                    break;
                case QMediaPlayer::NoMedia:
                    setLoading(false);
                    setSourceReady(false);
                    break;
                default:
                    break;
                }
            });

    connect(&m_importWatcher,
            &QFutureWatcher<MediaLibraryStorage::ImportBatchResult>::finished,
            this,
            [this]() {
        const MediaLibraryStorage::ImportBatchResult result = m_importWatcher.result();
        if (!result.importedPaths.isEmpty()) {
            const int firstRow = m_videos.size();
            const int lastRow = firstRow + result.importedPaths.size() - 1;
            beginInsertRows(QModelIndex(), firstRow, lastRow);
            for (const QString &filePath : result.importedPaths)
                appendImportedVideo(filePath);
            endInsertRows();
            emit libraryChanged();
        }

        setImporting(false);
        const QString message = result.importedPaths.isEmpty()
            ? QStringLiteral("没有导入新视频，可能已存在或格式不受支持")
            : QStringLiteral("已导入 %1 部视频，文件已复制到应用媒体库")
                  .arg(result.importedPaths.size());
        emit importFinished(result.importedPaths.size(), result.skippedCount, message);
    });

    m_progressSaveTimer.setInterval(5000);
    m_progressSaveTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_progressSaveTimer,
            &QTimer::timeout,
            this,
            [this]() { saveCurrentProgress(false); });
    m_progressSaveTimer.start();

    resolveAssets();
    m_currentIndex = qBound(0, m_currentIndex, qMax(0, m_videos.size() - 1));
    loadVideo(m_currentIndex, false);
}

VideoController::~VideoController()
{
    if (m_importWatcher.isRunning())
        m_importWatcher.waitForFinished();
    saveCurrentProgress(true);
    saveSettings();
}

int VideoController::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_videos.size();
}

QVariant VideoController::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_videos.size())
        return {};

    const VideoEntry &video = m_videos.at(index.row());
    switch (role) {
    case IdRole:
        return video.id;
    case TitleRole:
    case Qt::DisplayRole:
        return video.title;
    case SubtitleRole:
        return video.subtitle;
    case CategoryRole:
        return video.category;
    case DurationTextRole: {
        const qint64 knownDuration = m_knownDurations.value(video.id, 0);
        return knownDuration > 0 ? formatTime(knownDuration) : video.durationText;
    }
    case PosterRole:
        return video.poster;
    case AccentColorRole:
        return video.accentColor;
    case FavoriteRole:
        return video.favorite;
    case CurrentRole:
        return index.row() == m_currentIndex;
    case AvailableRole:
        return video.available;
    case ResumePositionRole:
        return m_resumePositions.value(video.id, 0);
    case ResumeProgressRole:
        return resumeProgressForRow(index.row());
    default:
        return {};
    }
}

QHash<int, QByteArray> VideoController::roleNames() const
{
    return {
        {IdRole, "videoId"},
        {TitleRole, "title"},
        {SubtitleRole, "subtitle"},
        {CategoryRole, "categoryName"},
        {DurationTextRole, "durationText"},
        {PosterRole, "posterSource"},
        {AccentColorRole, "accentColor"},
        {FavoriteRole, "favorite"},
        {CurrentRole, "current"},
        {AvailableRole, "available"},
        {ResumePositionRole, "resumePosition"},
        {ResumeProgressRole, "resumeProgress"}
    };
}

int VideoController::videoCount() const
{
    return m_videos.size();
}

int VideoController::currentIndex() const
{
    return m_currentIndex;
}

QString VideoController::currentId() const
{
    return currentVideo().id;
}

QString VideoController::title() const
{
    return currentVideo().title;
}

QString VideoController::subtitle() const
{
    return currentVideo().subtitle;
}

QString VideoController::category() const
{
    return currentVideo().category;
}

QUrl VideoController::posterSource() const
{
    return currentVideo().poster;
}

QString VideoController::accentColor() const
{
    return currentVideo().accentColor;
}

bool VideoController::currentAvailable() const
{
    return currentVideo().available;
}

void VideoController::attachVideoOutput(QObject *videoOutput)
{
    if (!videoOutput || m_player.videoOutput() == videoOutput)
        return;

    m_player.setVideoOutput(videoOutput);
}

void VideoController::detachVideoOutput(QObject *videoOutput)
{
    if (m_player.videoOutput() == videoOutput)
        m_player.setVideoOutput(nullptr);
}

bool VideoController::isPlaying() const
{
    return m_player.isPlaying();
}

bool VideoController::isLoading() const
{
    return m_loading;
}

bool VideoController::isSourceReady() const
{
    return m_sourceReady;
}

bool VideoController::isSeekable() const
{
    return m_player.isSeekable();
}

bool VideoController::isMuted() const
{
    return m_audioOutput.isMuted();
}

void VideoController::setMuted(bool muted)
{
    if (muted) {
        setVolume(0);
        return;
    }

    setVolume(m_volume > 0 ? m_volume : 1);
}

int VideoController::volume() const
{
    return m_volume;
}

void VideoController::setVolume(int volume)
{
    const int boundedVolume = qBound(0, volume, 10);
    const bool shouldMute = boundedVolume == 0;
    const bool volumeStateChanged = m_volume != boundedVolume;
    const bool muteStateChanged = m_audioOutput.isMuted() != shouldMute;

    if (!volumeStateChanged && !muteStateChanged)
        return;

    m_volume = boundedVolume;
    m_audioOutput.setVolume(static_cast<float>(m_volume) / 10.0F);

    if (muteStateChanged) {
        m_audioOutput.setMuted(shouldMute);
        emit mutedChanged();
        saveSettings();
    }

    if (volumeStateChanged)
        emit volumeChanged();
}

qint64 VideoController::position() const
{
    return m_player.position();
}

qint64 VideoController::duration() const
{
    return m_player.duration();
}

bool VideoController::autoPlayNext() const
{
    return m_autoPlayNext;
}

void VideoController::setAutoPlayNext(bool enabled)
{
    if (m_autoPlayNext == enabled)
        return;

    m_autoPlayNext = enabled;
    emit autoPlayNextChanged();
    saveSettings();
}

bool VideoController::importing() const
{
    return m_importing;
}

QString VideoController::libraryLocation() const
{
    return MediaLibraryStorage::videoDirectory();
}

QString VideoController::errorString() const
{
    return m_errorString;
}

void VideoController::play()
{
    if (!currentAvailable()) {
        const QString text = QStringLiteral("视频文件不存在，请确认 Videos 目录已复制到运行目录");
        setErrorString(text);
        emit playbackError(text);
        return;
    }

    if (m_player.mediaStatus() == QMediaPlayer::EndOfMedia)
        m_player.setPosition(0);

    if (!m_player.source().isEmpty()) {
        m_pendingPlay = false;
        m_player.play();
        return;
    }

    m_pendingPlay = true;
    if (m_player.source().isEmpty())
        loadVideo(m_currentIndex, true);
}

void VideoController::pause()
{
    m_pendingPlay = false;
    m_player.pause();
    saveCurrentProgress(true);
}

void VideoController::playPause()
{
    if (m_player.isPlaying())
        pause();
    else
        play();
}

void VideoController::next()
{
    if (m_videos.isEmpty())
        return;

    loadVideo((m_currentIndex + 1) % m_videos.size(), true);
}

void VideoController::previous()
{
    if (m_videos.isEmpty())
        return;

    if (m_player.position() > 3000) {
        seek(0);
        return;
    }

    const int target = (m_currentIndex - 1 + m_videos.size()) % m_videos.size();
    loadVideo(target, true);
}

void VideoController::selectVideo(int row)
{
    if (row < 0 || row >= m_videos.size())
        return;

    if (row == m_currentIndex) {
        playPause();
        return;
    }

    loadVideo(row, true);
}

void VideoController::seek(qint64 requestedPosition)
{
    if (!m_player.isSeekable())
        return;

    const qint64 boundedPosition = qBound<qint64>(0, requestedPosition, qMax<qint64>(0, duration()));
    m_player.setPosition(boundedPosition);
    m_resumePositions.insert(currentId(), boundedPosition);
    emitRowChanged(m_currentIndex, {ResumePositionRole, ResumeProgressRole});
}

void VideoController::toggleMute()
{
    setVolume((isMuted() || m_volume == 0) ? 1 : 0);
}

void VideoController::toggleFavorite(int row)
{
    if (row < 0 || row >= m_videos.size())
        return;

    m_videos[row].favorite = !m_videos.at(row).favorite;
    emitRowChanged(row, {FavoriteRole});
    saveSettings();
}

void VideoController::restartVideo(int row)
{
    if (row < 0 || row >= m_videos.size())
        return;

    m_resumePositions.insert(m_videos.at(row).id, 0);
    emitRowChanged(row, {ResumePositionRole, ResumeProgressRole});
    if (row == m_currentIndex)
        seek(0);
    saveSettings();
}

QString VideoController::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = qMax<qint64>(0, milliseconds / 1000);
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void VideoController::importFiles(const QVariantList &urls)
{
    if (m_importing)
        return;

    QList<QUrl> localUrls;
    localUrls.reserve(urls.size());
    for (const QVariant &value : urls) {
        const QUrl url = value.toUrl();
        if (url.isValid())
            localUrls.append(url);
    }

    if (localUrls.isEmpty()) {
        emit importFinished(0, 0, QStringLiteral("没有选择视频文件"));
        return;
    }

    setImporting(true);
    const QString destination = MediaLibraryStorage::videoDirectory();
    const QStringList extensions = supportedExtensions();
    m_importWatcher.setFuture(QtConcurrent::run(
        [localUrls, destination, extensions]() {
            return MediaLibraryStorage::importFiles(localUrls, destination, extensions);
        }));
}

const VideoController::VideoEntry &VideoController::currentVideo() const
{
    Q_ASSERT(!m_videos.isEmpty());
    return m_videos.at(m_currentIndex);
}

QString VideoController::locateVideoDirectory() const
{
    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    const QString currentDirectory = QDir::currentPath();
    const QStringList candidates = {
        QDir(applicationDirectory).filePath(QStringLiteral("Videos")),
        QDir(currentDirectory).filePath(QStringLiteral("Videos")),
        QDir(applicationDirectory).filePath(QStringLiteral("../Videos")),
        QDir(currentDirectory).filePath(QStringLiteral("../Videos")),
        QDir(applicationDirectory).filePath(QStringLiteral("../../Videos"))
    };

    for (const QString &candidate : candidates) {
        const QDir directory(QDir::cleanPath(candidate));
        if (directory.exists())
            return directory.absolutePath();
    }

    return {};
}

void VideoController::resolveAssets()
{
    const QString videoDirectory = locateVideoDirectory();

    for (VideoEntry &video : m_videos) {
        if (video.imported) {
            const QString importedPath = video.source.toLocalFile();
            video.available = QFileInfo::exists(importedPath);
            continue;
        }

        if (videoDirectory.isEmpty()) {
            video.available = false;
            continue;
        }

        const QString videoPath = QDir(videoDirectory).filePath(video.fileName);
        const QString posterPath = QDir(videoDirectory).filePath(video.posterFileName);
        video.available = QFileInfo::exists(videoPath);
        video.source = video.available ? QUrl::fromLocalFile(videoPath) : QUrl();
        video.poster = QFileInfo::exists(posterPath) ? QUrl::fromLocalFile(posterPath) : QUrl();
    }
}

void VideoController::loadImportedVideos()
{
    const QStringList files = MediaLibraryStorage::scanFiles(
        MediaLibraryStorage::videoDirectory(), supportedExtensions());
    for (const QString &filePath : files)
        appendImportedVideo(filePath);
}

void VideoController::appendImportedVideo(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists())
        return;

    for (const VideoEntry &video : m_videos) {
        if (video.source.isLocalFile()
            && QFileInfo(video.source.toLocalFile()).absoluteFilePath()
                == info.absoluteFilePath()) {
            return;
        }
    }

    const QByteArray idHash = QCryptographicHash::hash(
        info.fileName().toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    const QStringList accentColors = {
        QStringLiteral("#5C7CFF"), QStringLiteral("#21B8C7"),
        QStringLiteral("#B96CFF"), QStringLiteral("#FF8A5B"),
        QStringLiteral("#43A7FF"), QStringLiteral("#F15B8A")
    };

    VideoEntry video;
    video.id = QStringLiteral("local-%1").arg(QString::fromLatin1(idHash));
    video.title = info.completeBaseName();
    video.subtitle = QStringLiteral("用户导入的本地视频");
    video.category = QStringLiteral("本地导入");
    video.durationText = QStringLiteral("本地");
    video.fileName = info.fileName();
    video.accentColor = accentColors.at(m_videos.size() % accentColors.size());
    video.source = QUrl::fromLocalFile(info.absoluteFilePath());
    video.available = true;
    video.imported = true;
    m_videos.append(video);
}

void VideoController::loadSettings()
{
    QSettings settings;
    m_currentIndex = qMax(0, settings.value(QStringLiteral("videoCenter/lastIndex"), 0).toInt());
    m_autoPlayNext = settings.value(QStringLiteral("videoCenter/autoPlayNext"), true).toBool();
    m_audioOutput.setMuted(settings.value(QStringLiteral("videoCenter/muted"), false).toBool());

    const QStringList favoriteIds = settings.value(QStringLiteral("videoCenter/favorites")).toStringList();
    for (VideoEntry &video : m_videos)
        video.favorite = favoriteIds.contains(video.id);

    const QVariantMap storedPositions = settings.value(QStringLiteral("videoCenter/positions")).toMap();
    for (auto iterator = storedPositions.constBegin(); iterator != storedPositions.constEnd(); ++iterator)
        m_resumePositions.insert(iterator.key(), qMax<qint64>(0, iterator.value().toLongLong()));

    const QVariantMap storedDurations = settings.value(QStringLiteral("videoCenter/durations")).toMap();
    for (auto iterator = storedDurations.constBegin(); iterator != storedDurations.constEnd(); ++iterator)
        m_knownDurations.insert(iterator.key(), qMax<qint64>(0, iterator.value().toLongLong()));
}

void VideoController::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("videoCenter/lastIndex"), m_currentIndex);
    settings.setValue(QStringLiteral("videoCenter/autoPlayNext"), m_autoPlayNext);
    settings.setValue(QStringLiteral("videoCenter/muted"), m_audioOutput.isMuted());

    QStringList favoriteIds;
    for (const VideoEntry &video : m_videos) {
        if (video.favorite)
            favoriteIds.append(video.id);
    }
    settings.setValue(QStringLiteral("videoCenter/favorites"), favoriteIds);

    QVariantMap storedPositions;
    for (auto iterator = m_resumePositions.constBegin(); iterator != m_resumePositions.constEnd(); ++iterator)
        storedPositions.insert(iterator.key(), iterator.value());
    settings.setValue(QStringLiteral("videoCenter/positions"), storedPositions);

    QVariantMap storedDurations;
    for (auto iterator = m_knownDurations.constBegin(); iterator != m_knownDurations.constEnd(); ++iterator)
        storedDurations.insert(iterator.key(), iterator.value());
    settings.setValue(QStringLiteral("videoCenter/durations"), storedDurations);
}

void VideoController::saveCurrentProgress(bool force)
{
    if (m_videos.isEmpty())
        return;

    const qint64 currentPosition = position();
    m_resumePositions.insert(currentId(), currentPosition);

    if (!force && m_lastPersistedPosition >= 0
        && qAbs(currentPosition - m_lastPersistedPosition) < kProgressWriteStepMs) {
        return;
    }

    m_lastPersistedPosition = currentPosition;
    saveSettings();
}

void VideoController::loadVideo(int index, bool autoPlay, bool restart)
{
    if (index < 0 || index >= m_videos.size())
        return;

    const int previousIndex = m_currentIndex;
    if (!m_player.source().isEmpty())
        saveCurrentProgress(true);

    m_pendingPlay = autoPlay;
    m_player.stop();
    m_currentIndex = index;
    m_lastPersistedPosition = -1;
    m_lastProgressPercent = -1;
    setLoading(false);
    setSourceReady(false);
    setErrorString(QString());

    if (restart)
        m_resumePositions.insert(currentId(), 0);

    m_pendingResumePosition = restart ? 0 : m_resumePositions.value(currentId(), 0);

    if (!currentAvailable()) {
        m_player.setSource(QUrl());
        setErrorString(QStringLiteral("未找到 %1").arg(currentVideo().fileName));
    } else {
        setLoading(true);
        m_player.setSource(currentVideo().source);
    }

    if (previousIndex != m_currentIndex)
        emitRowChanged(previousIndex, {CurrentRole, ResumePositionRole, ResumeProgressRole});
    emitRowChanged(m_currentIndex, {CurrentRole, ResumePositionRole, ResumeProgressRole});
    emit currentVideoChanged();

    if (autoPlay)
        play();
}

void VideoController::setLoading(bool loading)
{
    if (m_loading == loading)
        return;

    m_loading = loading;
    emit loadingChanged();
}

void VideoController::setSourceReady(bool ready)
{
    if (m_sourceReady == ready)
        return;

    m_sourceReady = ready;
    emit sourceReadyChanged();
}

void VideoController::setImporting(bool importing)
{
    if (m_importing == importing)
        return;
    m_importing = importing;
    emit importingChanged();
}

QStringList VideoController::supportedExtensions()
{
    return {QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
            QStringLiteral("avi"), QStringLiteral("webm"), QStringLiteral("m4v"),
            QStringLiteral("wmv")};
}

void VideoController::setErrorString(const QString &message)
{
    if (m_errorString == message)
        return;

    m_errorString = message;
    emit errorStringChanged();
}

void VideoController::emitRowChanged(int row, const QList<int> &roles)
{
    if (row < 0 || row >= m_videos.size())
        return;

    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}

qreal VideoController::resumeProgressForRow(int row) const
{
    if (row < 0 || row >= m_videos.size())
        return 0.0;

    const VideoEntry &video = m_videos.at(row);
    const qint64 storedPosition = m_resumePositions.value(video.id, 0);

    const qint64 knownDuration = row == m_currentIndex && duration() > 0
        ? duration()
        : m_knownDurations.value(video.id, video.imported ? 0 : 8000);
    if (knownDuration <= 0)
        return 0.0;
    return qBound<qreal>(0.0,
                         static_cast<qreal>(storedPosition) / knownDuration,
                         1.0);
}
