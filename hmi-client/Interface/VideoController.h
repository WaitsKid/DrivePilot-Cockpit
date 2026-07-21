#ifndef VIDEOCONTROLLER_H
#define VIDEOCONTROLLER_H

#include "MediaLibraryStorage.h"

#include <QAbstractListModel>
#include <QAudioOutput>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVector>
#include <qqmlintegration.h>

class VideoController : public QAbstractListModel
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(VideoCenter)

    Q_PROPERTY(int videoCount READ videoCount NOTIFY libraryChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentVideoChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentVideoChanged)
    Q_PROPERTY(QString title READ title NOTIFY currentVideoChanged)
    Q_PROPERTY(QString subtitle READ subtitle NOTIFY currentVideoChanged)
    Q_PROPERTY(QString category READ category NOTIFY currentVideoChanged)
    Q_PROPERTY(QUrl posterSource READ posterSource NOTIFY currentVideoChanged)
    Q_PROPERTY(QString accentColor READ accentColor NOTIFY currentVideoChanged)
    Q_PROPERTY(bool currentAvailable READ currentAvailable NOTIFY currentVideoChanged)

    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool sourceReady READ isSourceReady NOTIFY sourceReadyChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool autoPlayNext READ autoPlayNext WRITE setAutoPlayNext NOTIFY autoPlayNextChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString libraryLocation READ libraryLocation CONSTANT)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        SubtitleRole,
        CategoryRole,
        DurationTextRole,
        PosterRole,
        AccentColorRole,
        FavoriteRole,
        CurrentRole,
        AvailableRole,
        ResumePositionRole,
        ResumeProgressRole
    };
    Q_ENUM(Role)

    explicit VideoController(QObject *parent = nullptr);
    ~VideoController() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int videoCount() const;
    int currentIndex() const;
    QString currentId() const;
    QString title() const;
    QString subtitle() const;
    QString category() const;
    QUrl posterSource() const;
    QString accentColor() const;
    bool currentAvailable() const;

    bool isPlaying() const;
    bool isLoading() const;
    bool isSourceReady() const;
    bool isSeekable() const;
    bool isMuted() const;
    void setMuted(bool muted);
    int volume() const;
    void setVolume(int volume);
    qint64 position() const;
    qint64 duration() const;
    bool autoPlayNext() const;
    void setAutoPlayNext(bool enabled);
    bool importing() const;
    QString libraryLocation() const;
    QString errorString() const;

    Q_INVOKABLE void attachVideoOutput(QObject *videoOutput);
    Q_INVOKABLE void detachVideoOutput(QObject *videoOutput);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void selectVideo(int row);
    Q_INVOKABLE void seek(qint64 position);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void toggleFavorite(int row);
    Q_INVOKABLE void restartVideo(int row);
    Q_INVOKABLE QString formatTime(qint64 milliseconds) const;
    Q_INVOKABLE void importFiles(const QVariantList &urls);

signals:
    void libraryChanged();
    void currentVideoChanged();
    void playingChanged();
    void loadingChanged();
    void sourceReadyChanged();
    void seekableChanged();
    void mutedChanged();
    void volumeChanged();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void autoPlayNextChanged();
    void importingChanged();
    void importFinished(int importedCount, int skippedCount, const QString &message);
    void errorStringChanged();
    void playbackError(const QString &message);

private:
    struct VideoEntry {
        QString id;
        QString title;
        QString subtitle;
        QString category;
        QString durationText;
        QString fileName;
        QString posterFileName;
        QString accentColor;
        QUrl source;
        QUrl poster;
        bool favorite = false;
        bool available = false;
        bool imported = false;
    };

    const VideoEntry &currentVideo() const;
    QString locateVideoDirectory() const;
    void resolveAssets();
    void loadImportedVideos();
    void appendImportedVideo(const QString &filePath);
    void loadSettings();
    void saveSettings() const;
    void saveCurrentProgress(bool force = false);
    void loadVideo(int index, bool autoPlay, bool restart = false);
    void setLoading(bool loading);
    void setSourceReady(bool ready);
    void setErrorString(const QString &message);
    void setImporting(bool importing);
    static QStringList supportedExtensions();
    void emitRowChanged(int row, const QList<int> &roles);
    qreal resumeProgressForRow(int row) const;

    QVector<VideoEntry> m_videos;
    QAudioOutput m_audioOutput;
    QMediaPlayer m_player;
    QTimer m_progressSaveTimer;
    QHash<QString, qint64> m_resumePositions;
    QHash<QString, qint64> m_knownDurations;
    QFutureWatcher<MediaLibraryStorage::ImportBatchResult> m_importWatcher;

    int m_currentIndex = 0;
    int m_volume = 7;
    bool m_loading = false;
    bool m_sourceReady = false;
    bool m_pendingPlay = false;
    bool m_autoPlayNext = true;
    bool m_importing = false;
    qint64 m_pendingResumePosition = 0;
    qint64 m_lastPersistedPosition = -1;
    int m_lastProgressPercent = -1;
    QString m_errorString;
};

#endif
