#include "Playlist.h"
#include <QDebug>

Playlist::Playlist(QObject *parent)
    : QObject(parent), m_currentIndex(-1) {}

void Playlist::addTrack(const QString& filePath) {
    tracks.emplace_back(filePath);
    emit trackAdded(tracks.size() - 1);
    qDebug() << "Track added:" << filePath;
}

void Playlist::addTracks(const QStringList& filePaths) {
    for (const QString& path : filePaths) {
        addTrack(path);
    }
}

void Playlist::removeTrack(int index) {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        tracks.erase(tracks.begin() + index);
        if (m_currentIndex == index) {
            m_currentIndex = -1;
        } else if (m_currentIndex > index) {
            m_currentIndex--;
        }
        emit trackRemoved(index);
    }
}

void Playlist::clear() {
    tracks.clear();
    m_currentIndex = -1;
    emit playlistCleared();
}

Track Playlist::getTrack(int index) const {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        return tracks[index];
    }
    return Track("");
}

QString Playlist::getTrackPath(int index) const {
    if (index >= 0 && index < static_cast<int>(tracks.size())) {
        return tracks[index].filePath;
    }
    return "";
}

void Playlist::setCurrentIndex(int index) {
    if (index >= -1 && index < static_cast<int>(tracks.size())) {
        m_currentIndex = index;
        emit currentTrackChanged(index);
    }
}

bool Playlist::hasNext() const {
    return m_currentIndex < static_cast<int>(tracks.size()) - 1;
}

bool Playlist::hasPrevious() const {
    return m_currentIndex > 0;
}

void Playlist::next() {
    if (hasNext()) {
        setCurrentIndex(m_currentIndex + 1);
    }
}

void Playlist::previous() {
    if (hasPrevious()) {
        setCurrentIndex(m_currentIndex - 1);
    }
}

QStringList Playlist::getAllPaths() const {
    QStringList paths;
    for (const auto& track : tracks) {
        paths << track.filePath;
    }
    return paths;
}
