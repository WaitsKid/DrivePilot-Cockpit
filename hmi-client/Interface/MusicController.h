#ifndef MUSICCONTROLLER_H
#define MUSICCONTROLLER_H

#include "MediaLibraryStorage.h"

#include <QAudioOutput>
#include <QFutureWatcher>
#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVector>
#include <qqmlintegration.h>

class MusicController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(MusicPlayer)

    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY playlistChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentTrackChanged)
    Q_PROPERTY(QString title READ title NOTIFY currentTrackChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY currentTrackChanged)
    Q_PROPERTY(QString album READ album NOTIFY currentTrackChanged)
    Q_PROPERTY(QString primaryColor READ primaryColor NOTIFY currentTrackChanged)
    Q_PROPERTY(QString secondaryColor READ secondaryColor NOTIFY currentTrackChanged)
    Q_PROPERTY(int coverVariant READ coverVariant NOTIFY currentTrackChanged)

    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(bool sourceReady READ isSourceReady NOTIFY sourceReadyChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString libraryLocation READ libraryLocation CONSTANT)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit MusicController(QObject *parent = nullptr);
    ~MusicController() override;

    QVariantList playlist() const;
    int trackCount() const;
    int currentIndex() const;

    QString title() const;
    QString artist() const;
    QString album() const;
    QString primaryColor() const;
    QString secondaryColor() const;
    int coverVariant() const;

    bool isPlaying() const;
    bool isMuted() const;
    void setMuted(bool muted);
    int volume() const;
    void setVolume(int volume);
    qint64 position() const;
    qint64 duration() const;
    bool isSeekable() const;
    bool isSourceReady() const;
    bool importing() const;
    QString libraryLocation() const;
    QString errorString() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void selectTrack(int index);
    Q_INVOKABLE void seek(qint64 position);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void importFiles(const QVariantList &urls);

signals:
    void playlistChanged();
    void currentTrackChanged();
    void playingChanged();
    void mutedChanged();
    void volumeChanged();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void seekableChanged();
    void sourceReadyChanged();
    void importingChanged();
    void importFinished(int importedCount, int skippedCount, const QString &message);
    void errorStringChanged();
    void playbackError(const QString &message);

private:
    struct Track {
        QString title;
        QString artist;
        QString album;
        QString resourcePath;
        QString fileName;
        QString primaryColor;
        QString secondaryColor;
        int coverVariant = 0;
        QString durationText;
        QUrl playableSource;
        bool imported = false;
    };

    const Track &currentTrack() const;
    void loadTrack(int index, bool autoPlay);
    void prepareAudioFiles();
    void loadImportedTracks();
    void appendImportedTrack(const QString &filePath);
    QUrl materializeResource(const Track &track) const;
    void setErrorString(const QString &message);
    void setSourceReady(bool ready);
    void setImporting(bool importing);
    static QStringList supportedExtensions();

    QVector<Track> m_tracks;
    QAudioOutput m_audioOutput;
    QMediaPlayer m_player;
    QFutureWatcher<MediaLibraryStorage::ImportBatchResult> m_importWatcher;
    int m_currentIndex = 0;
    int m_volume = 7;
    bool m_sourceReady = false;
    bool m_pendingPlay = false;
    bool m_importing = false;
    QString m_errorString;
};

#endif
