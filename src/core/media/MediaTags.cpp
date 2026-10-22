#include "MediaTags.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace {

// --- вспомогательное --------------------------------------------------------

quint32 readBE32(const QByteArray &b, int offset)
{
    return (quint32(quint8(b.at(offset))) << 24) | (quint32(quint8(b.at(offset + 1))) << 16)
           | (quint32(quint8(b.at(offset + 2))) << 8) | quint32(quint8(b.at(offset + 3)));
}

quint32 readLE32(const QByteArray &b, int offset)
{
    return quint32(quint8(b.at(offset))) | (quint32(quint8(b.at(offset + 1))) << 8)
           | (quint32(quint8(b.at(offset + 2))) << 16) | (quint32(quint8(b.at(offset + 3))) << 24);
}

quint32 readSyncSafe32(const QByteArray &b, int offset)
{
    return (quint32(quint8(b.at(offset)) & 0x7F) << 21)
           | (quint32(quint8(b.at(offset + 1)) & 0x7F) << 14)
           | (quint32(quint8(b.at(offset + 2)) & 0x7F) << 7)
           | quint32(quint8(b.at(offset + 3)) & 0x7F);
}

/// Строка в кодировке ID3v2: 0 = latin1, 1 = utf-16 с BOM, 2 = utf-16be, 3 = utf-8.
QString decodeId3String(const QByteArray &raw, int encoding)
{
    if (raw.isEmpty())
        return QString();

    switch (encoding) {
    case 1:
        return QString::fromUtf8(raw).normalized(QString::NormalizationForm_C); // BOM обработает Qt
    case 2: {
        QString out = QString::fromUtf16(reinterpret_cast<const char16_t *>(raw.constData()),
                                         raw.size() / 2);
        return out;
    }
    case 3:
        return QString::fromUtf8(raw);
    default:
        return QString::fromLatin1(raw);
    }
}

QByteArray stripTrailingNul(QByteArray data, int encoding)
{
    if (encoding == 1 || encoding == 2) {
        while (data.size() >= 2 && data.at(data.size() - 1) == '\0'
               && data.at(data.size() - 2) == '\0')
            data.chop(2);
    } else {
        while (!data.isEmpty() && data.at(data.size() - 1) == '\0')
            data.chop(1);
    }
    return data;
}

QString cleaned(const QString &text)
{
    QString out = text;
    out.replace(QLatin1Char('\0'), QLatin1Char(' '));
    return out.trimmed();
}

QString containerOf(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix;
}

// --- FLAC -------------------------------------------------------------------

struct FlacTags {
    QString title, artist, album, genre, date;
    qint64 durationMs = 0;
    QByteArray coverData;
    QString coverMime;
};

void parseVorbisComment(const QByteArray &block, FlacTags *tags)
{
    if (block.size() < 8)
        return;

    int offset = 0;
    const quint32 vendorLength = readLE32(block, offset);
    offset += 4;
    if (vendorLength > quint32(block.size()))
        return;
    offset += int(vendorLength);
    if (offset + 4 > block.size())
        return;

    const quint32 count = readLE32(block, offset);
    offset += 4;

    for (quint32 i = 0; i < count && offset + 4 <= block.size(); ++i) {
        const quint32 length = readLE32(block, offset);
        offset += 4;
        if (int(length) > block.size() - offset)
            break;

        const QByteArray entry = block.mid(offset, int(length));
        offset += int(length);

        const int eq = entry.indexOf('=');
        if (eq <= 0)
            continue;

        const QString key = QString::fromLatin1(entry.left(eq)).trimmed().toUpper();
        const QString value = cleaned(QString::fromUtf8(entry.mid(eq + 1)));

        if (key == QLatin1String("TITLE"))
            tags->title = value;
        else if (key == QLatin1String("ARTIST") || key == QLatin1String("ALBUMARTIST"))
            tags->artist = tags->artist.isEmpty() ? value : tags->artist;
        else if (key == QLatin1String("ALBUM"))
            tags->album = value;
        else if (key == QLatin1String("GENRE"))
            tags->genre = value;
        else if (key == QLatin1String("DATE"))
            tags->date = value;
    }
}

void parseFlacPicture(const QByteArray &block, FlacTags *tags)
{
    if (tags->coverData.size() || block.size() < 32)
        return;

    const quint32 mimeLength = readBE32(block, 4);
    if (mimeLength > quint32(block.size()))
        return;
    const int mimeEnd = 8 + int(mimeLength);
    const QString mime = QString::fromLatin1(block.mid(8, int(mimeLength)));

    if (mimeEnd + 4 > block.size())
        return;
    const quint32 descriptionLength = readBE32(block, mimeEnd);
    const int dataLengthOffset = mimeEnd + 4 + int(descriptionLength) + 16;
    if (dataLengthOffset + 4 > block.size())
        return;

    const quint32 dataLength = readBE32(block, dataLengthOffset);
    const int dataOffset = dataLengthOffset + 4;
    if (int(dataLength) > block.size() - dataOffset)
        return;

    tags->coverData = block.mid(dataOffset, int(dataLength));
    tags->coverMime = mime;
}

FlacTags parseFlac(const QByteArray &file)
{
    FlacTags tags;
    if (file.size() < 8 || file.left(4) != QByteArrayLiteral("fLaC"))
        return tags;

    int offset = 4;
    bool last = false;
    while (!last && offset + 4 <= file.size()) {
        const quint8 header = quint8(file.at(offset));
        last = (header & 0x80) != 0;
        const quint8 type = header & 0x7F;
        const int length = (int(quint8(file.at(offset + 1))) << 16)
                           | (int(quint8(file.at(offset + 2))) << 8) | int(quint8(file.at(offset + 3)));
        offset += 4;
        if (length < 0 || offset + length > file.size())
            break;
        const QByteArray block = file.mid(offset, length);
        offset += length;

        switch (type) {
        case 0: { // STREAMINFO
            if (block.size() < 34)
                break;
            const quint32 sampleRate = (quint32(quint8(block.at(10))) << 12)
                                       | (quint32(quint8(block.at(11))) << 4)
                                       | (quint32(quint8(block.at(12))) >> 4);
            const quint64 totalSamples = (quint64(quint8(block.at(13)) & 0x0F) << 32)
                                         | (quint64(quint8(block.at(14))) << 24)
                                         | (quint64(quint8(block.at(15))) << 16)
                                         | (quint64(quint8(block.at(16))) << 8)
                                         | quint64(quint8(block.at(17)));
            if (sampleRate > 0 && totalSamples > 0)
                tags.durationMs = qint64(totalSamples * 1000 / sampleRate);
            break;
        }
        case 4: // VORBIS_COMMENT
            parseVorbisComment(block, &tags);
            break;
        case 6: // PICTURE
            parseFlacPicture(block, &tags);
            break;
        default:
            break;
        }
    }
    return tags;
}

// --- MP3 / ID3v2 ------------------------------------------------------------

struct Mp3Tags {
    QString title, artist, album, genre, date;
    qint64 durationMs = 0;
    QByteArray coverData;
    QString coverMime;
    int audioOffset = 0;
};

void parseId3v2Tech(const QByteArray &file, int major, int tagEnd, Mp3Tags *tags)
{
    const bool v22 = (major == 2);
    const int idSize = v22 ? 3 : 4;
    const int frameHeaderSize = v22 ? 6 : 10;

    int offset = 10;
    while (offset + frameHeaderSize <= tagEnd && offset + frameHeaderSize <= file.size()) {
        const QByteArray id = file.mid(offset, idSize);
        if (id.at(0) == '\0')
            break; // padding

        int frameSize = 0;
        if (v22)
            frameSize = (int(quint8(file.at(offset + 3))) << 16)
                        | (int(quint8(file.at(offset + 4))) << 8) | int(quint8(file.at(offset + 5)));
        else if (major == 4)
            frameSize = int(readSyncSafe32(file, offset + 4));
        else
            frameSize = int(readBE32(file, offset + 4));

        if (frameSize <= 0 || offset + frameHeaderSize + frameSize > file.size())
            break;

        const QByteArray payload = file.mid(offset + frameHeaderSize, frameSize);
        offset += frameHeaderSize + frameSize;
        if (payload.isEmpty())
            continue;

        const int encoding = quint8(payload.at(0));
        const QByteArray body = payload.mid(1);

        const auto isText = [&id](const char *v23, const char *v22id) {
            return id == QByteArray(v23) || (v22id && id == QByteArray(v22id));
        };

        if (isText("TIT2", "TT2"))
            tags->title = cleaned(decodeId3String(stripTrailingNul(body, encoding), encoding));
        else if (isText("TPE1", "TP1"))
            tags->artist = cleaned(decodeId3String(stripTrailingNul(body, encoding), encoding));
        else if (isText("TALB", "TAL"))
            tags->album = cleaned(decodeId3String(stripTrailingNul(body, encoding), encoding));
        else if (isText("TCON", "TCO"))
            tags->genre = cleaned(decodeId3String(stripTrailingNul(body, encoding), encoding));
        else if (isText("TYER", nullptr) || isText("TDRC", nullptr))
            tags->date = cleaned(decodeId3String(stripTrailingNul(body, encoding), encoding));
        else if (isText("APIC", "PIC")) {
            // APIC: encoding, mime (latin1, NUL), тип картинки, описание, данные.
            int cursor = 0;
            if (!v22) {
                const int mimeEnd = body.indexOf('\0', cursor);
                if (mimeEnd < 0)
                    continue;
                const QString mime = QString::fromLatin1(body.mid(cursor, mimeEnd - cursor));
                cursor = mimeEnd + 1;
                if (cursor >= body.size())
                    continue;
                ++cursor; // picture type
                if (encoding == 1 || encoding == 2) {
                    const int descEnd = body.indexOf(QByteArray(2, '\0'), cursor);
                    cursor = (descEnd < 0) ? body.size() : descEnd + 2;
                } else {
                    const int descEnd = body.indexOf('\0', cursor);
                    cursor = (descEnd < 0) ? body.size() : descEnd + 1;
                }
                if (cursor < body.size() && tags->coverData.isEmpty()) {
                    tags->coverData = body.mid(cursor);
                    tags->coverMime = mime;
                }
            } else {
                // ID3v2.2: PIC: encoding, 3 байта формата, тип, описание, данные.
                if (body.size() < 5)
                    continue;
                const QByteArray format = body.mid(0, 3).toLower();
                int picCursor = 4;
                const int descEnd = body.indexOf('\0', picCursor);
                picCursor = (descEnd < 0) ? body.size() : descEnd + 1;
                if (picCursor < body.size() && tags->coverData.isEmpty()) {
                    tags->coverData = body.mid(picCursor);
                    tags->coverMime = (format == QByteArrayLiteral("png"))
                                          ? QStringLiteral("image/png")
                                          : QStringLiteral("image/jpeg");
                }
            }
        }
    }
}

/// Оценка длительности по первому кадру MPEG (без Xing/VBRI — приблизительно).
qint64 estimateMp3DurationMs(const QByteArray &file, int start)
{
    static const int bitrateV1L3[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224,
                                        256, 320, 0};
    static const int bitrateV2L3[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144,
                                        160, 0};

    for (int i = start; i + 4 < file.size(); ++i) {
        if (quint8(file.at(i)) != 0xFF || (quint8(file.at(i + 1)) & 0xE0) != 0xE0)
            continue;

        const quint8 b1 = quint8(file.at(i + 1));
        const quint8 b2 = quint8(file.at(i + 2));
        const int versionBits = (b1 >> 3) & 0x03; // 3 = MPEG1, 2 = MPEG2, 0 = MPEG2.5
        const int layerBits = (b1 >> 1) & 0x03;   // 1 = Layer III
        if (versionBits == 1 || layerBits == 0)
            continue;

        const int bitrateIndex = (b2 >> 4) & 0x0F;
        if (bitrateIndex == 0 || bitrateIndex == 15)
            continue;

        const bool mpeg1 = (versionBits == 3);
        const int kbps = mpeg1 ? bitrateV1L3[bitrateIndex] : bitrateV2L3[bitrateIndex];
        if (kbps <= 0)
            continue;

        const qint64 audioBytes = file.size() - start;
        return audioBytes * 8 * 1000 / (qint64(kbps) * 1000);
    }
    return 0;
}

Mp3Tags parseMp3(const QByteArray &file)
{
    Mp3Tags tags;
    if (file.size() > 10 && file.startsWith(QByteArrayLiteral("ID3"))) {
        const int major = quint8(file.at(3));
        const int flags = quint8(file.at(5));
        const int size = int(readSyncSafe32(file, 6));
        int tagEnd = 10 + size;
        if (flags & 0x10) // footer
            tagEnd += 10;
        tagEnd = qMin(tagEnd, file.size());
        parseId3v2Tech(file, major, tagEnd, &tags);
        tags.audioOffset = tagEnd;
    }
    tags.durationMs = estimateMp3DurationMs(file, tags.audioOffset);
    return tags;
}

// --- WAV --------------------------------------------------------------------

qint64 parseWavDurationMs(const QByteArray &file)
{
    if (file.size() < 44 || !file.startsWith(QByteArrayLiteral("RIFF")))
        return 0;

    int offset = 12;
    int channels = 0, bits = 0, sampleRate = 0;
    qint64 dataSize = 0;

    while (offset + 8 <= file.size()) {
        const QByteArray id = file.mid(offset, 4);
        const quint32 size = readLE32(file, offset + 4);
        const int bodyOffset = offset + 8;

        if (id == QByteArrayLiteral("fmt ") && bodyOffset + 16 <= file.size()) {
            channels = int(quint16(quint8(file.at(bodyOffset + 2))
                                   | (quint16(quint8(file.at(bodyOffset + 3))) << 8)));
            sampleRate = int(readLE32(file, bodyOffset + 4));
            bits = int(quint16(quint8(file.at(bodyOffset + 14))
                               | (quint16(quint8(file.at(bodyOffset + 15))) << 8)));
        } else if (id == QByteArrayLiteral("data")) {
            dataSize = qint64(size);
            if (dataSize <= 0 || dataSize > file.size() - bodyOffset)
                dataSize = file.size() - bodyOffset;
            break;
        }
        offset = bodyOffset + int(size) + (int(size) % 2);
    }

    const qint64 bytesPerSecond = qint64(sampleRate) * channels * (bits / 8);
    if (bytesPerSecond <= 0 || dataSize <= 0)
        return 0;
    return dataSize * 1000 / bytesPerSecond;
}

/// Читает не весь файл: для MP3 хватает хвоста тега + начала аудио.
QByteArray readHead(const QString &path, qint64 limit)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.read(limit);
}

} // namespace

namespace MediaTags {

QString Info::durationText() const
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

QImage Info::coverImage() const
{
    if (coverData.isEmpty())
        return QImage();
    QImage image;
    image.loadFromData(coverData);
    return image;
}

qint64 flacDurationMs(const QByteArray &file, bool *ok)
{
    const FlacTags tags = parseFlac(file);
    if (ok)
        *ok = tags.durationMs > 0;
    return tags.durationMs;
}

Info read(const QString &filePath)
{
    Info info;
    info.filePath = filePath;
    info.container = containerOf(filePath);
    info.title = QFileInfo(filePath).fileName();
    info.artist = QStringLiteral("Unknown");
    info.genericTitle = true;

    if (info.title.startsWith(QLatin1String(".")))
        return info;
    if (info.container == QLatin1String("flac")) {
        const FlacTags tags = parseFlac(readHead(filePath, 1 << 21)); // 2 МБ: теги + обложка
        info.durationMs = tags.durationMs;
        info.coverData = tags.coverData;
        info.coverMime = tags.coverMime;
        if (!tags.title.isEmpty()) {
            info.title = tags.title;
            info.genericTitle = false;
        }
        if (!tags.artist.isEmpty())
            info.artist = tags.artist;
        info.album = tags.album;
        info.genre = tags.genre;
        info.year = tags.date.left(4).toInt();
    } else if (info.container == QLatin1String("mp3")) {
        const Mp3Tags tags = parseMp3(readHead(filePath, 1 << 21));
        info.durationMs = tags.durationMs;
        info.coverData = tags.coverData;
        info.coverMime = tags.coverMime;
        if (!tags.title.isEmpty()) {
            info.title = tags.title;
            info.genericTitle = false;
        }
        if (!tags.artist.isEmpty())
            info.artist = tags.artist;
        info.album = tags.album;
        info.genre = tags.genre;
        info.year = tags.date.left(4).toInt();
    } else if (info.container == QLatin1String("wav")) {
        // Головы достаточно, если data-чанк в начале; иначе дочитываем файл.
        info.durationMs = parseWavDurationMs(readHead(filePath, 256 * 1024));
        if (info.durationMs == 0) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly))
                info.durationMs = parseWavDurationMs(file.readAll());
        }
    }

    // Страховка: у части файлов тег есть, но пустой.
    if (info.title.isEmpty())
        info.title = QFileInfo(filePath).fileName();
    if (info.artist.isEmpty())
        info.artist = QStringLiteral("Unknown");
    return info;
}

} // namespace MediaTags