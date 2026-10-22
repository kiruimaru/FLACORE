#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>

/**
 * Один трек плейлиста: путь и то, что удалось прочитать из файла.
 * Метаданные читаются один раз при добавлении (MediaTags), обложка хранится
 * уменьшенной — чтобы сотни треков не съели память.
 */
struct Track {
    QString filePath;
    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;
    QImage cover;

    explicit Track(const QString &path = QString());

    QString durationText() const;   // «3:45» / «1:02:03» / «--:--»
    bool hasMetadata() const;
};

/**
 * Список треков и текущая позиция — единственный владелец состояния плейлиста.
 * Представление (LibraryPage) подписано на сигналы, поэтому «убрать», «очистить»
 * и смена трека не дублируются в UI и не расходятся с моделью.
 */
class Playlist : public QObject {
    Q_OBJECT

public:
    explicit Playlist(QObject *parent = nullptr);

    void addTrack(const QString& filePath);
    void addTracks(const QStringList& filePaths);
    void removeTrack(int index);
    void clear();

    int count() const { return int(tracks.size()); }
    Track getTrack(int index) const;
    QString getTrackPath(int index) const;

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    bool hasNext() const;
    bool hasPrevious() const;
    /// Переход вперёд: если ничего не выбрано, начинает с первого трека.
    void next();
    void previous();

    QStringList getAllPaths() const;

signals:
    void trackAdded(int index);
    void trackRemoved(int index);
    void currentTrackChanged(int index);
    void playlistCleared();

private:
    std::vector<Track> tracks;
    int m_currentIndex;
};

#endif // PLAYLIST_H