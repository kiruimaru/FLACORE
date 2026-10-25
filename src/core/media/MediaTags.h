#ifndef MEDIATAGS_H
#define MEDIATAGS_H

#include <QByteArray>
#include <QImage>
#include <QString>

/**
 * Минимальное чтение метаданных из аудиофайлов без внешних библиотек.
 *
 * Поддержано:
 *   FLAC  — STREAMINFO (длительность), VORBIS_COMMENT (title/artist/album/date/genre),
 *           PICTURE (обложка);
 *   MP3   — ID3v2.2/2.3/2.4 (TIT2/TT2, TPE1/TP1, TALB/TAL, TYER/TDRC, TCON/TCO, APIC/PIC),
 *           длительность оценивается по первому кадру MPEG;
 *   WAV   — fmt + data (длительность).
 *
 * Не поддержано (осознанно): MP4/M4A (нужен разбор атомов), Ogg Vorbis/Opus
 * (нужен разбор страниц) — для них вернутся defaults, интерфейс покажет имя файла.
 */
namespace MediaTags {

struct Info {
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString genre;
    int year = 0;
    qint64 durationMs = 0;
    QByteArray coverData;   // сырые байты изображения (как в файле)
    QString coverMime;      // image/jpeg, image/png, ...
    QString container;      // "flac", "mp3", "wav", "" — для отладки/подсказок
    bool genericTitle = true;   // true, если title взят из имени файла, а не из тега

    bool hasCover() const { return !coverData.isEmpty(); }
    QImage coverImage() const;  // декодирует coverData (пустая QImage, если нет)

    /// Человекочитаемая длительность «3:45» / «1:02:03», «--:--» если неизвестна.
    QString durationText() const;
};

/// Читает теги. Никогда не бросает: при любой проблеме возвращает то, что успел
/// прочитать, с title = имени файла и artist = "Unknown".
Info read(const QString &filePath);

/// Длительность в миллисекундах только для уже открытого буфера (для тестов).
qint64 flacDurationMs(const QByteArray &file, bool *ok = nullptr);

/// Установка обложки в аудиофайл (Issue #14): вшивает картинку через ffmpeg
/// и сохраняет обложку рядом с файлом.
bool saveCoverToFile(const QString &audioPath, const QImage &coverImage,
                     const QString &ffmpegDir = QString());

} // namespace MediaTags

#endif // MEDIATAGS_H