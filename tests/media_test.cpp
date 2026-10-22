// Тест чтения метаданных и палитры обложки: фикстуры FLAC и MP3 собираются
// в коде (байт за байтом), поэтому тест не зависит ни от сети, ни от файлов
// примеров в репозитории.
#include <QBuffer>
#include <QCoreApplication>
#include <QImage>
#include <QTemporaryDir>
#include <cstdio>

#include "core/media/CoverPalette.h"
#include "core/media/MediaTags.h"
#include "core/player/Playlist.h"

namespace {

int g_failures = 0;

void check(bool condition, const QString &what)
{
    std::printf("  %s %s\n", condition ? "ok  " : "FAIL", what.toUtf8().constData());
    if (!condition)
        ++g_failures;
}

void note(const QString &what)
{
    std::printf("       %s\n", what.toUtf8().constData());
}

void putBE32(QByteArray &out, quint32 value)
{
    out.append(char((value >> 24) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char(value & 0xFF));
}

void putLE32(QByteArray &out, quint32 value)
{
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 24) & 0xFF));
}

void putSyncSafe32(QByteArray &out, quint32 value)
{
    out.append(char((value >> 21) & 0x7F));
    out.append(char((value >> 14) & 0x7F));
    out.append(char((value >> 7) & 0x7F));
    out.append(char(value & 0x7F));
}

QByteArray pngBytes()
{
    QImage image(4, 4, QImage::Format_ARGB32);
    image.fill(QColor(0xd9, 0x2b, 0x2b));
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

/// FLAC с STREAMINFO (3 секунды), VORBIS_COMMENT и PICTURE.
QByteArray buildFlac(const QByteArray &cover)
{
    QByteArray out;
    out.append("fLaC");

    // STREAMINFO: 44100 Гц, 2 канала, 16 бит, 132300 сэмплов = 3 с.
    QByteArray streamInfo;
    streamInfo.append(char(0x10)).append(char(0x00));           // min block
    streamInfo.append(char(0x10)).append(char(0x00));           // max block
    streamInfo.append(QByteArray(3, '\0'));                     // min frame
    streamInfo.append(QByteArray(3, '\0'));                     // max frame
    const quint32 sampleRate = 44100;
    const quint64 totalSamples = 132300;
    streamInfo.append(char((sampleRate >> 12) & 0xFF));
    streamInfo.append(char((sampleRate >> 4) & 0xFF));
    streamInfo.append(char(((sampleRate & 0x0F) << 4) | (1 << 1) | 0x00));
    streamInfo.append(char(0xF0 | char((totalSamples >> 32) & 0x0F)));
    streamInfo.append(char((totalSamples >> 24) & 0xFF));
    streamInfo.append(char((totalSamples >> 16) & 0xFF));
    streamInfo.append(char((totalSamples >> 8) & 0xFF));
    streamInfo.append(char(totalSamples & 0xFF));
    streamInfo.append(QByteArray(16, '\0'));                    // MD5
    out.append(char(0x00));                                     // не последний, тип 0
    out.append(char((streamInfo.size() >> 16) & 0xFF));
    out.append(char((streamInfo.size() >> 8) & 0xFF));
    out.append(char(streamInfo.size() & 0xFF));
    out.append(streamInfo);

    // VORBIS_COMMENT
    QByteArray comments;
    const QByteArray vendor = "FLACORE test";
    putLE32(comments, quint32(vendor.size()));
    comments.append(vendor);
    const QList<QByteArray> entries = {"TITLE=Test Tone", "ARTIST=Unit Tester",
                                       "ALBUM=Fixtures", "DATE=2026"};
    putLE32(comments, quint32(entries.size()));
    for (const QByteArray &entry : entries) {
        putLE32(comments, quint32(entry.size()));
        comments.append(entry);
    }
    out.append(char(0x04));
    out.append(char((comments.size() >> 16) & 0xFF));
    out.append(char((comments.size() >> 8) & 0xFF));
    out.append(char(comments.size() & 0xFF));
    out.append(comments);

    // PICTURE (последний блок)
    QByteArray picture;
    putBE32(picture, 3);                       // front cover
    const QByteArray mime = "image/png";
    putBE32(picture, quint32(mime.size()));
    picture.append(mime);
    putBE32(picture, 0);                       // описание
    putBE32(picture, 4);                       // ширина
    putBE32(picture, 4);                       // высота
    putBE32(picture, 32);                      // глубина
    putBE32(picture, 0);                       // цвета
    putBE32(picture, quint32(cover.size()));
    picture.append(cover);
    out.append(char(0x80 | 0x06));
    out.append(char((picture.size() >> 16) & 0xFF));
    out.append(char((picture.size() >> 8) & 0xFF));
    out.append(char(picture.size() & 0xFF));
    out.append(picture);

    return out;
}

/// MP3: ID3v2.3 (TIT2/TPE1/TALB/APIC) + один кадр MPEG1 Layer III 128 kbps.
QByteArray buildMp3(const QByteArray &cover)
{
    QByteArray frames;

    const auto textFrame = [&frames](const char *id, const QByteArray &text) {
        QByteArray payload;
        payload.append(char(3));               // utf-8
        payload.append(text);
        frames.append(id, 4);
        putBE32(frames, quint32(payload.size()));
        frames.append(QByteArray(2, '\0'));    // флаги кадра
        frames.append(payload);
    };

    textFrame("TIT2", "Mp3 Tone");
    textFrame("TPE1", "Mp3 Artist");
    textFrame("TALB", "Mp3 Album");

    QByteArray apic;
    apic.append(char(3));
    apic.append("image/png");
    apic.append(char(0));
    apic.append(char(3));                      // front cover
    apic.append(char(0));                      // пустое описание
    apic.append(cover);
    frames.append("APIC", 4);
    putBE32(frames, quint32(apic.size()));
    frames.append(QByteArray(2, '\0'));
    frames.append(apic);

    QByteArray out;
    out.append("ID3");
    out.append(char(3)).append(char(0)).append(char(0));
    putSyncSafe32(out, quint32(frames.size()));
    out.append(frames);

    // Кадр: 0xFF 0xFB 0x90 0x00 -> MPEG1 Layer III, 128 kbps, 44100 Гц.
    const QByteArray frameHeader = QByteArray::fromHex("FFFB9000");
    out.append(frameHeader);
    out.append(QByteArray(5000, '\0'));
    return out;
}

/// Простой WAV: 8000 Гц, моно, 8 бит, 1 секунда.
QByteArray buildWav()
{
    QByteArray out;
    const quint32 dataSize = 8000;
    out.append("RIFF");
    putLE32(out, 36 + dataSize);
    out.append("WAVE");
    out.append("fmt ");
    putLE32(out, 16);
    out.append(char(1)).append(char(0));       // PCM
    out.append(char(1)).append(char(0));       // каналы
    putLE32(out, 8000);                        // частота
    putLE32(out, 8000);                        // байт/с
    out.append(char(1)).append(char(0));       // выравнивание
    out.append(char(8)).append(char(0));       // бит на сэмпл
    out.append("data");
    putLE32(out, dataSize);
    out.append(QByteArray(int(dataSize), char(0x80)));
    return out;
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(data) == data.size();
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTemporaryDir dir;
    if (!dir.isValid())
        return 2;

    const QByteArray cover = pngBytes();

    std::printf("Теги и палитра\n");

    // --- FLAC ---------------------------------------------------------------
    const QString flacPath = dir.filePath(QStringLiteral("tone.flac"));
    check(writeFile(flacPath, buildFlac(cover)), QStringLiteral("фикстура FLAC записана"));
    {
        const MediaTags::Info info = MediaTags::read(flacPath);
        check(info.container == QLatin1String("flac"), QStringLiteral("контейнер определён как flac"));
        check(info.title == QLatin1String("Test Tone"), QStringLiteral("TITLE из VORBIS_COMMENT"));
        check(info.artist == QLatin1String("Unit Tester"), QStringLiteral("ARTIST из VORBIS_COMMENT"));
        check(info.album == QLatin1String("Fixtures"), QStringLiteral("ALBUM из VORBIS_COMMENT"));
        check(info.durationMs == 3000, QStringLiteral("длительность из STREAMINFO = 3000 мс"));
        check(info.durationText() == QLatin1String("0:03"), QStringLiteral("длительность в тексте: 0:03"));
        check(info.hasCover(), QStringLiteral("обложка найдена в PICTURE"));
        check(!info.coverImage().isNull(), QStringLiteral("обложка декодируется в картинку"));
        check(info.coverMime.contains(QLatin1String("png")), QStringLiteral("mime обложки = image/png"));
    }

    // --- MP3 ----------------------------------------------------------------
    const QString mp3Path = dir.filePath(QStringLiteral("tone.mp3"));
    const QByteArray mp3Bytes = buildMp3(cover);
    check(writeFile(mp3Path, mp3Bytes), QStringLiteral("фикстура MP3 записана"));
    {
        const MediaTags::Info info = MediaTags::read(mp3Path);
        if (info.title != QLatin1String("Mp3 Tone")) {
            QString dump;
            for (int i = 0; i < qMin(24, mp3Bytes.size()); ++i)
                dump += QStringLiteral("%1 ").arg(quint8(mp3Bytes.at(i)), 2, 16, QLatin1Char('0'));
            note(QStringLiteral("первые байты MP3: %1").arg(dump));
        }
        check(info.container == QLatin1String("mp3"), QStringLiteral("контейнер определён как mp3"));
        check(info.title == QLatin1String("Mp3 Tone"), QStringLiteral("TIT2 прочитан"));
        check(info.artist == QLatin1String("Mp3 Artist"), QStringLiteral("TPE1 прочитан"));
        check(info.album == QLatin1String("Mp3 Album"), QStringLiteral("TALB прочитан"));
        check(info.durationMs >= 200 && info.durationMs <= 400,
              QStringLiteral("длительность оценена по кадру (~312 мс), получено %1")
                  .arg(info.durationMs));
        check(!info.coverImage().isNull(), QStringLiteral("APIC декодируется"));
    }

    // --- WAV ----------------------------------------------------------------
    const QString wavPath = dir.filePath(QStringLiteral("tone.wav"));
    check(writeFile(wavPath, buildWav()), QStringLiteral("фикстура WAV записана"));
    {
        const MediaTags::Info info = MediaTags::read(wavPath);
        check(info.durationMs == 1000, QStringLiteral("длительность WAV = 1000 мс, получено %1")
                                           .arg(info.durationMs));
        check(info.genericTitle && info.title == QLatin1String("tone.wav"),
              QStringLiteral("без тегов имя файла становится названием"));
        check(info.artist == QLatin1String("Unknown"), QStringLiteral("исполнитель по умолчанию Unknown"));
    }

    // --- мусор на входе -----------------------------------------------------
    const QString bogusPath = dir.filePath(QStringLiteral("broken.flac"));
    check(writeFile(bogusPath, QByteArray("не flac вовсе")), QStringLiteral("битый файл записан"));
    {
        const MediaTags::Info info = MediaTags::read(bogusPath);
        check(info.title == QLatin1String("broken.flac"), QStringLiteral("битый файл не ломает чтение"));
        check(info.durationMs == 0, QStringLiteral("у битого файла длительность 0"));
    }

    // --- плейлист поверх тегов ---------------------------------------------
    {
        Playlist playlist;
        int added = 0;
        QObject::connect(&playlist, &Playlist::trackAdded, [&added](int) { ++added; });
        playlist.addTracks({flacPath, mp3Path, wavPath});
        check(playlist.count() == 3 && added == 3, QStringLiteral("три трека добавлены с сигналами"));

        const Track first = playlist.getTrack(0);
        check(first.title == QLatin1String("Test Tone"), QStringLiteral("трек берёт название из тега"));
        check(first.album == QLatin1String("Fixtures"), QStringLiteral("трек берёт альбом из тега"));
        check(first.durationText() == QLatin1String("0:03"),
              QStringLiteral("длительность трека %1 (ожидалось 0:03)")
                  .arg(first.durationText()));
        check(!first.cover.isNull(), QStringLiteral("обложка трека уменьшена и сохранена"));
        check(first.cover.width() <= 360, QStringLiteral("обложка не больше 360 px"));
    }

    // --- палитра обложки ----------------------------------------------------
    {
        QImage image(48, 48, QImage::Format_ARGB32);
        image.fill(QColor::fromHsv(210, 200, 190));   // синий
        const CoverPalette::Palette palette = CoverPalette::extract(image);
        check(palette.valid, QStringLiteral("палитра извлечена из однотонной обложки"));
        const int hue = palette.accent.hsvHue();
        check(qAbs(hue - 210) <= 25, QStringLiteral("оттенок акцента рядом с исходным (%1)").arg(hue));
        check(palette.accent.hsvSaturation() >= 90, QStringLiteral("акцент достаточно насыщен"));
        check(palette.background.value() < 40, QStringLiteral("фон из палитры тёмный"));

        const CoverPalette::Palette empty = CoverPalette::extract(QImage());
        check(!empty.valid, QStringLiteral("пустая картинка не даёт палитру"));
    }

    std::printf("\n%s\n", g_failures == 0 ? "ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ" : "ЕСТЬ ПРОВАЛЫ");
    std::printf("Провалено: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}