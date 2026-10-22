#include "Playlist.h"

#include "core/media/MediaTags.h"

#include <QFileInfo>

namespace {

/// Обложка хранится уменьшенной: полноразмерные картинки в памяти не нужны.
constexpr int kCoverLimit = 360;

QImage scaledCover(const QImage &source)
{
    if (source.isNull())
        return QImage();
    if (source.width() <= kCoverLimit && source.height() <= kCoverLimit)
        return source;
    return source.scaled(kCoverLimit, kCoverLimit, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
}

} // namespace

Track::Track(const QString &path)
{
    if (path.isEmpty())
        return;

    const MediaTags::Info info = MediaTags::read(path);
    filePath = path;
    title = info.title;
    artist = info.artist;
    album = info.album;
    durationMs = info.durationMs;

    if (info.hasCover())
        cover = scaledCover(info.coverImage());

    if (title.isEmpty())
        title = QFileInfo(path).fileName();
    if (artist.isEmpty())
        artist = QStringLiteral("Unknown");
}

QString Track::durationText() const
{
    if (durationMs <= 0)
        return QStringLiteral("--:--");

    const qint64 totalSeconds = durationMs / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    const auto two = [](qint64 value) {
        return QString::number(value).rightJustified(2, QLatin1Char('0'));
    };

    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours).arg(two(minutes), two(seconds));
    return QStringLiteral("%1:%2").arg(minutes).arg(two(seconds));
}

bool Track::hasMetadata() const
{
    return durationMs > 0 || artist != QLatin1String("Unknown") || !album.isEmpty();
}

Playlist::Playlist(QObject *parent)
    : QObject(parent), m_currentIndex(-1) {}

void Playlist::addTrack(const QString& filePath)
{
    if (filePath.isEmpty())
        return;

    tracks.emplace_back(filePath);
    emit trackAdded(int(tracks.size()) - 1);
}

void Playlist::addTracks(const QStringList& filePaths)
{
    for (const QString& path : filePaths) {
        addTrack(path);
    }
}

void Playlist::removeTrack(int index)
{
    if (index < 0 || index >= int(tracks.size()))
        return;

    const int previousIndex = m_currentIndex;

    tracks.erase(tracks.begin() + index);

    const bool removedCurrent = (m_currentIndex == index);
    if (removedCurrent) {
        m_currentIndex = -1;
    } else if (m_currentIndex > index) {
        m_currentIndex--;
    }

    emit trackRemoved(index);

    // Об изменении текущего трека узнают все: и когда удалили играющий,
    // и когда индекс играющего сдвинулся из-за удаления трека выше.
    if (m_currentIndex != previousIndex)
        emit currentTrackChanged(m_currentIndex);
}

void Playlist::clear()
{
    if (tracks.empty() && m_currentIndex == -1)
        return;

    tracks.clear();
    m_currentIndex = -1;
    emit playlistCleared();
    emit currentTrackChanged(m_currentIndex);
}

Track Playlist::getTrack(int index) const
{
    if (index >= 0 && index < int(tracks.size())) {
        return tracks[index];
    }
    return Track();
}

QString Playlist::getTrackPath(int index) const
{
    if (index >= 0 && index < int(tracks.size())) {
        return tracks[index].filePath;
    }
    return QString();
}

void Playlist::setCurrentIndex(int index)
{
    if (index < -1 || index >= int(tracks.size()))
        return;
    if (m_currentIndex == index)
        return;

    m_currentIndex = index;
    emit currentTrackChanged(index);
}

bool Playlist::hasNext() const
{
    if (tracks.empty())
        return false;
    if (m_currentIndex < 0)
        return true;   // ничего не выбрано — следующий = первый
    return m_currentIndex < int(tracks.size()) - 1;
}

bool Playlist::hasPrevious() const
{
    return m_currentIndex > 0;
}

void Playlist::next()
{
    if (!hasNext())
        return;
    setCurrentIndex(m_currentIndex < 0 ? 0 : m_currentIndex + 1);
}

void Playlist::previous()
{
    if (!hasPrevious())
        return;
    setCurrentIndex(m_currentIndex - 1);
}

QStringList Playlist::getAllPaths() const
{
    QStringList paths;
    paths.reserve(int(tracks.size()));
    for (const auto& track : tracks) {
        paths << track.filePath;
    }
    return paths;
}