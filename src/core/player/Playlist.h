#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <vector>

struct Track {
    QString filePath;
    QString title;
    QString artist;
    qint64 duration;

    Track(const QString& path)
        : filePath(path),
          title(QFileInfo(path).fileName()),
          artist("Unknown"),
          duration(0) {}
};

class Playlist : public QObject {
    Q_OBJECT

public:
    explicit Playlist(QObject *parent = nullptr);

    void addTrack(const QString& filePath);
    void addTracks(const QStringList& filePaths);
    void removeTrack(int index);
    void clear();

    int count() const { return tracks.size(); }
    Track getTrack(int index) const;
    QString getTrackPath(int index) const;

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    bool hasNext() const;
    bool hasPrevious() const;
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
