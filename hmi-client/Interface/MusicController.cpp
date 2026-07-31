#include "MusicController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVariantMap>
#include <QtConcurrent>
#include <QtGlobal>

namespace {
const QStringList kPrimaryColors = {
    QStringLiteral("#5C7CFF"), QStringLiteral("#25C8A8"),
    QStringLiteral("#FF8A5B"), QStringLiteral("#B96CFF"),
    QStringLiteral("#43A7FF"), QStringLiteral("#F15B8A")
};
const QStringList kSecondaryColors = {
    QStringLiteral("#B24DFF"), QStringLiteral("#246BFD"),
    QStringLiteral("#F3C84B"), QStringLiteral("#5C7CFF"),
    QStringLiteral("#52E6FB"), QStringLiteral("#FFB74D")
};
}

MusicController::MusicController(QObject *parent)
    : QObject(parent)
    , m_tracks({
          {QStringLiteral("Neon Highway"), QStringLiteral("Aurora Circuit"),
           QStringLiteral("Drive Sessions"), QStringLiteral(":/Audio/neon_highway.wav"),
           QStringLiteral("neon_highway.wav"), QStringLiteral("#5C7CFF"),
           QStringLiteral("#B24DFF"), 0, QStringLiteral("00:36"), {}, false},
          {QStringLiteral("Night Signal"), QStringLiteral("Lumen Echo"),
           QStringLiteral("Drive Sessions"), QStringLiteral(":/Audio/night_signal.wav"),
           QStringLiteral("night_signal.wav"), QStringLiteral("#25C8A8"),
           QStringLiteral("#246BFD"), 1, QStringLiteral("00:38"), {}, false},
          {QStringLiteral("Coastal Pulse"), QStringLiteral("Blue Meridian"),
           QStringLiteral("Drive Sessions"), QStringLiteral(":/Audio/coastal_pulse.wav"),
           QStringLiteral("coastal_pulse.wav"), QStringLiteral("#FF8A5B"),
           QStringLiteral("#F3C84B"), 2, QStringLiteral("00:40"), {}, false}
      })
{
    m_player.setAudioOutput(&m_audioOutput);
    m_audioOutput.setVolume(static_cast<float>(m_volume) / 10.0F);

    connect(&m_player, &QMediaPlayer::playingChanged,
            this, [this](bool) { emit playingChanged(); });
    connect(&m_player, &QMediaPlayer::positionChanged,
            this, &MusicController::positionChanged);
    connect(&m_player, &QMediaPlayer::durationChanged,
            this, &MusicController::durationChanged);
    connect(&m_player, &QMediaPlayer::seekableChanged,
            this, [this](bool) { emit seekableChanged(); });
    connect(&m_player, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error, const QString &message) {
        const QString text = message.trimmed().isEmpty()
            ? QStringLiteral("音频播放失败，请检查媒体格式或 Qt Multimedia 后端")
            : message.trimmed();
        setErrorString(text);
        emit playbackError(text);
    });
    connect(&m_player, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        switch (status) {
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            setSourceReady(true);
            if (m_pendingPlay) {
                m_pendingPlay = false;
                m_player.play();
            }
            break;
        case QMediaPlayer::EndOfMedia:
            setSourceReady(true);
            if (!m_tracks.isEmpty())
                loadTrack((m_currentIndex + 1) % m_tracks.size(), true);
            break;
        case QMediaPlayer::InvalidMedia:
            m_pendingPlay = false;
            setSourceReady(false);
            break;
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::StalledMedia:
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
        for (const QString &filePath : result.importedPaths)
            appendImportedTrack(filePath);

        if (!result.importedPaths.isEmpty())
            emit playlistChanged();

        setImporting(false);
        const QString message = result.importedPaths.isEmpty()
            ? QStringLiteral("没有导入新音乐，可能已存在或格式不受支持")
            : QStringLiteral("已导入 %1 首音乐，文件已复制到应用媒体库")
                  .arg(result.importedPaths.size());
        emit importFinished(result.importedPaths.size(), result.skippedCount, message);
    });

    prepareAudioFiles();
    loadImportedTracks();
    loadTrack(0, false);
}

MusicController::~MusicController()
{
    if (m_importWatcher.isRunning())
        m_importWatcher.waitForFinished();
}

QVariantList MusicController::playlist() const
{
    QVariantList result;
    result.reserve(m_tracks.size());
    for (int index = 0; index < m_tracks.size(); ++index) {
        const Track &track = m_tracks.at(index);
        QVariantMap item;
        item.insert(QStringLiteral("index"), index);
        item.insert(QStringLiteral("title"), track.title);
        item.insert(QStringLiteral("artist"), track.artist);
        item.insert(QStringLiteral("album"), track.album);
        item.insert(QStringLiteral("primaryColor"), track.primaryColor);
        item.insert(QStringLiteral("secondaryColor"), track.secondaryColor);
        item.insert(QStringLiteral("coverVariant"), track.coverVariant);
        item.insert(QStringLiteral("durationText"), track.durationText);
        item.insert(QStringLiteral("imported"), track.imported);
        result.append(item);
    }
    return result;
}

int MusicController::trackCount() const { return m_tracks.size(); }
int MusicController::currentIndex() const { return m_currentIndex; }
QString MusicController::title() const { return currentTrack().title; }
QString MusicController::artist() const { return currentTrack().artist; }
QString MusicController::album() const { return currentTrack().album; }
QString MusicController::primaryColor() const { return currentTrack().primaryColor; }
QString MusicController::secondaryColor() const { return currentTrack().secondaryColor; }
int MusicController::coverVariant() const { return currentTrack().coverVariant; }
bool MusicController::isPlaying() const { return m_player.isPlaying(); }
bool MusicController::isMuted() const { return m_audioOutput.isMuted(); }

void MusicController::setMuted(bool muted)
{
    if (m_audioOutput.isMuted() == muted)
        return;
    m_audioOutput.setMuted(muted);
    emit mutedChanged();
}

int MusicController::volume() const { return m_volume; }

void MusicController::setVolume(int volume)
{
    const int boundedVolume = qBound(0, volume, 10);
    if (m_volume == boundedVolume)
        return;
    m_volume = boundedVolume;
    m_audioOutput.setVolume(static_cast<float>(m_volume) / 10.0F);
    emit volumeChanged();
}

qint64 MusicController::position() const { return m_player.position(); }
qint64 MusicController::duration() const { return m_player.duration(); }
bool MusicController::isSeekable() const { return m_player.isSeekable(); }
bool MusicController::isSourceReady() const { return m_sourceReady; }
bool MusicController::importing() const { return m_importing; }
QString MusicController::libraryLocation() const { return MediaLibraryStorage::musicDirectory(); }
QString MusicController::errorString() const { return m_errorString; }

void MusicController::play()
{
    if (m_tracks.isEmpty())
        return;
    if (m_player.mediaStatus() == QMediaPlayer::EndOfMedia)
        m_player.setPosition(0);
    if (m_player.mediaStatus() == QMediaPlayer::LoadedMedia
        || m_player.mediaStatus() == QMediaPlayer::BufferedMedia) {
        m_pendingPlay = false;
        m_player.play();
        return;
    }
    m_pendingPlay = true;
    if (m_player.source().isEmpty())
        loadTrack(m_currentIndex, true);
}

void MusicController::pause() { m_pendingPlay = false; m_player.pause(); }
void MusicController::playPause() { m_player.isPlaying() ? pause() : play(); }

void MusicController::next()
{
    if (!m_tracks.isEmpty())
        loadTrack((m_currentIndex + 1) % m_tracks.size(), m_player.isPlaying());
}

void MusicController::previous()
{
    if (m_tracks.isEmpty())
        return;
    if (m_player.position() > 3000) {
        m_player.setPosition(0);
        return;
    }
    const int target = (m_currentIndex - 1 + m_tracks.size()) % m_tracks.size();
    loadTrack(target, m_player.isPlaying());
}

void MusicController::selectTrack(int index)
{
    if (index < 0 || index >= m_tracks.size())
        return;
    if (index == m_currentIndex) {
        play();
        return;
    }
    loadTrack(index, true);
}

void MusicController::seek(qint64 requestedPosition)
{
    if (m_player.isSeekable())
        m_player.setPosition(qBound<qint64>(0, requestedPosition, m_player.duration()));
}

void MusicController::toggleMute() { setMuted(!isMuted()); }

void MusicController::importFiles(const QVariantList &urls)
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
        emit importFinished(0, 0, QStringLiteral("没有选择音乐文件"));
        return;
    }

    setImporting(true);
    const QString destination = MediaLibraryStorage::musicDirectory();
    const QStringList extensions = supportedExtensions();
    m_importWatcher.setFuture(QtConcurrent::run(
        [localUrls, destination, extensions]() {
            return MediaLibraryStorage::importFiles(localUrls, destination, extensions);
        }));
}

const MusicController::Track &MusicController::currentTrack() const
{
    Q_ASSERT(!m_tracks.isEmpty());
    return m_tracks.at(m_currentIndex);
}

void MusicController::loadTrack(int index, bool autoPlay)
{
    if (index < 0 || index >= m_tracks.size())
        return;
    m_pendingPlay = autoPlay;
    m_player.stop();
    m_currentIndex = index;
    setSourceReady(false);
    setErrorString(QString());
    m_player.setSource(currentTrack().playableSource);
    emit currentTrackChanged();
    if (autoPlay)
        play();
}

void MusicController::prepareAudioFiles()
{
    for (Track &track : m_tracks)
        track.playableSource = materializeResource(track);
}

void MusicController::loadImportedTracks()
{
    const QStringList files = MediaLibraryStorage::scanFiles(
        MediaLibraryStorage::musicDirectory(), supportedExtensions());
    for (const QString &filePath : files)
        appendImportedTrack(filePath);
}

void MusicController::appendImportedTrack(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists())
        return;

    for (const Track &track : m_tracks) {
        if (track.playableSource.isLocalFile()
            && QFileInfo(track.playableSource.toLocalFile()).absoluteFilePath()
                == info.absoluteFilePath()) {
            return;
        }
    }

    const int paletteIndex = m_tracks.size() % kPrimaryColors.size();
    Track track;
    track.title = info.completeBaseName();
    track.artist = QStringLiteral("本地音乐");
    track.album = QStringLiteral("用户媒体库");
    track.fileName = info.fileName();
    track.primaryColor = kPrimaryColors.at(paletteIndex);
    track.secondaryColor = kSecondaryColors.at(paletteIndex);
    track.coverVariant = m_tracks.size() % 3;
    track.durationText = QStringLiteral("本地");
    track.playableSource = QUrl::fromLocalFile(info.absoluteFilePath());
    track.imported = true;
    m_tracks.append(track);
}

QUrl MusicController::materializeResource(const Track &track) const
{
    const QUrl resourceUrl(QStringLiteral("qrc%1").arg(track.resourcePath));
    QFile resource(track.resourcePath);
    if (!resource.open(QIODevice::ReadOnly))
        return resourceUrl;

    const qint64 resourceSize = resource.size();
    resource.close();
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty())
        return resourceUrl;

    const QString audioDirectory = QDir(cacheRoot).filePath(QStringLiteral("audio"));
    if (!QDir().mkpath(audioDirectory))
        return resourceUrl;

    const QString outputPath = QDir(audioDirectory).filePath(track.fileName);
    const QFileInfo outputInfo(outputPath);
    if (!outputInfo.exists() || outputInfo.size() != resourceSize) {
        QFile::remove(outputPath);
        if (!QFile::copy(track.resourcePath, outputPath))
            return resourceUrl;
    }
    return QFileInfo::exists(outputPath) ? QUrl::fromLocalFile(outputPath) : resourceUrl;
}

void MusicController::setSourceReady(bool ready)
{
    if (m_sourceReady == ready)
        return;
    m_sourceReady = ready;
    emit sourceReadyChanged();
}

void MusicController::setImporting(bool importing)
{
    if (m_importing == importing)
        return;
    m_importing = importing;
    emit importingChanged();
}

void MusicController::setErrorString(const QString &message)
{
    if (m_errorString == message)
        return;
    m_errorString = message;
    emit errorStringChanged();
}

QStringList MusicController::supportedExtensions()
{
    return {QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("m4a"),
            QStringLiteral("aac"), QStringLiteral("flac"), QStringLiteral("ogg"),
            QStringLiteral("opus"), QStringLiteral("wma")};
}
