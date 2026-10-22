// Автотест «живой подборки»: разбор чартов Deezer и Apple Music, ссылки
// источников, «исполнитель — название» для поиска и чтение самого чарта
// SoundCloud через заглушку yt-dlp (tests/fake_ytdlp.cpp).
//
// Сеть не нужна: ответы источников — фикстуры, yt-dlp подменён заглушкой.
#include "core/online/Charts.h"
#include "core/ytdlp/YtDlp.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QThread>
#include <cstdio>

#ifndef FAKE_YTDLP_PATH
#error "FAKE_YTDLP_PATH не задан (см. tests/CMakeLists.txt)"
#endif

namespace {

int g_failures = 0;

void check(bool condition, const char *what, int line)
{
    if (condition) {
        std::printf("  ok   %s\n", what);
    } else {
        std::printf("  FAIL %s (строка %d)\n", what, line);
        ++g_failures;
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

template <typename Predicate>
bool waitFor(Predicate predicate, int timeoutMs = 20000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }
    return predicate();
}

// Ответ Deezer в том виде, в каком его отдаёт api.deezer.com/chart/0/tracks.
const char *kDeezerJson = R"({
  "data": [
    {
      "id": 1,
      "title": "Blinding Lights",
      "duration": 200,
      "link": "https://www.deezer.com/track/1",
      "preview": "https://cdns-preview-1.dzcdn.net/stream/track-1.mp3",
      "artist": {"name": "The Weeknd", "picture_medium": "https://api.deezer.com/artist/1/picture_medium.jpg"},
      "album": {"title": "After Hours", "cover_medium": "https://api.deezer.com/album/1/cover_medium.jpg"}
    },
    {
      "id": 2,
      "title": "Второй трек",
      "duration": 245,
      "link": "https://www.deezer.com/track/2",
      "preview": "",
      "artist": {"name": "Исполнитель", "picture_medium": "https://api.deezer.com/artist/2/artist_picture.jpg"},
      "album": {"title": "", "cover_medium": ""}
    }
  ]
})";

// Ответ Apple Music: rss.applemarketingtools.com/.../most-played/N/songs.json
const char *kAppleJson = R"({
  "feed": {
    "title": "Top Songs",
    "results": [
      {
        "name": "Espresso",
        "artistName": "Sabrina Carpenter",
        "collectionName": "Short n' Sweet",
        "url": "https://music.apple.com/us/album/espresso/1",
        "previewUrl": "https://audio-ssl.itunes.apple.com/preview/espresso.m4a",
        "artworkUrl100": "https://is1-ssl.mzstatic.com/image/thumb/Feature/100x100bb.jpg",
        "durationInMillis": 175000
      },
      {
        "name": "Новинка",
        "artistName": "Артист",
        "url": "https://music.apple.com/ru/album/2",
        "previewUrl": "",
        "artworkUrl100": "https://is1-ssl.mzstatic.com/image/thumb/Feature2/100x100bb.jpg"
      }
    ]
  }
})";

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("charts-selftest"));
    QStandardPaths::setTestModeEnabled(true);

    std::printf("Чарты: разбор источников, ссылки и чтение чарта SoundCloud\n");

    // --- 1. Deezer: разбор топ-чарта ---------------------------------------
    std::printf("\nDeezer:\n");
    {
        QString error;
        const QList<ChartTrack> tracks = parseDeezerChart(QByteArray(kDeezerJson), &error);

        CHECK(error.isEmpty());
        CHECK(tracks.size() == 2);

        if (tracks.size() == 2) {
            const ChartTrack &first = tracks.at(0);
            CHECK(first.title == QLatin1String("Blinding Lights"));
            CHECK(first.artist == QLatin1String("The Weeknd"));
            CHECK(first.album == QLatin1String("After Hours"));
            CHECK(first.durationMs == 200000);
            CHECK(first.durationText() == QLatin1String("3:20"));
            CHECK(first.sourceName() == QLatin1String("Deezer"));
            CHECK(first.url == QLatin1String("https://www.deezer.com/track/1"));
            CHECK(first.searchQuery() == QLatin1String("The Weeknd - Blinding Lights"));
            CHECK(first.previewUrl.startsWith(QLatin1String("https://cdns-preview-1")));
            CHECK(first.coverUrl.endsWith(QLatin1String("cover_medium.jpg")));

            // Без обложки альбома берётся фото исполнителя — иначе в полке пусто.
            CHECK(tracks.at(1).coverUrl.endsWith(QLatin1String("artist_picture.jpg")));
            CHECK(tracks.at(1).previewUrl.isEmpty());
        }

        QString deezerError;
        CHECK(parseDeezerChart(QByteArray(R"({"error":{"type":"DataException",)"
                                          R"("message":"Invalid genre"}})"),
                               &deezerError)
                  .isEmpty());
        CHECK(deezerError.contains(QLatin1String("Invalid genre")));

        QString brokenError;
        CHECK(parseDeezerChart(QByteArray("{ это не json"), &brokenError).isEmpty());
        CHECK(!brokenError.isEmpty());
    }

    // --- 2. Apple Music -----------------------------------------------------
    std::printf("\nApple Music:\n");
    {
        QString error;
        const QList<ChartTrack> tracks = parseAppleChart(QByteArray(kAppleJson), &error);

        CHECK(error.isEmpty());
        CHECK(tracks.size() == 2);

        if (tracks.size() == 2) {
            const ChartTrack &first = tracks.at(0);
            CHECK(first.title == QLatin1String("Espresso"));
            CHECK(first.artist == QLatin1String("Sabrina Carpenter"));
            CHECK(first.sourceName() == QLatin1String("Apple Music"));
            CHECK(first.durationMs == 175000);
            CHECK(first.durationText() == QLatin1String("2:55"));
            CHECK(first.url.startsWith(QLatin1String("https://music.apple.com/")));
            CHECK(first.searchQuery().startsWith(QLatin1String("Sabrina Carpenter - Espresso")));
            // 100x100 заменяется на 600x600: в полке нужна крупнее.
            CHECK(first.coverUrl.contains(QLatin1String("600x600bb")));
            CHECK(tracks.at(1).durationMs == 0);
            CHECK(tracks.at(1).durationText() == QLatin1String("--:--"));
        }

        QString appleError;
        CHECK(parseAppleChart(QByteArray(R"({"feed":{"results":[]}})"), &appleError).isEmpty());
        CHECK(!appleError.isEmpty());
    }

    // --- 3. Ссылки источников ----------------------------------------------
    std::printf("\nСсылки источников:\n");
    CHECK(ChartsFetcher::deezerUrl(QString(), 30)
          == QLatin1String("https://api.deezer.com/chart/0/tracks?limit=30"));
    CHECK(ChartsFetcher::deezerUrl(QStringLiteral("132"), 30)
          == QLatin1String("https://api.deezer.com/chart/132/tracks?limit=30"));
    CHECK(ChartsFetcher::deezerUrl(QStringLiteral("132"), 5000).endsWith(QLatin1String("limit=100")));
    CHECK(ChartsFetcher::appleUrl(QStringLiteral("new-music"), QStringLiteral("ru"), 30)
          == QLatin1String("https://rss.applemarketingtools.com/api/v2/ru/music/new-music/30/songs.json"));
    CHECK(ChartsFetcher::appleUrl(QString(), QString(), 0)
          == QLatin1String("https://rss.applemarketingtools.com/api/v2/us/music/most-played/1/songs.json"));
    CHECK(soundCloudChartUrl(QStringLiteral("electronic"))
          == QLatin1String("https://soundcloud.com/charts/top?genre=electronic"));
    CHECK(soundCloudChartUrl(QString()).endsWith(QLatin1String("genre=all-music")));

    // --- 4. Справочники -----------------------------------------------------
    std::printf("\nСправочники:\n");
    const QList<ChartSource> genres = deezerGenres();
    CHECK(genres.size() >= 10);
    CHECK(!genres.isEmpty() && genres.first().id == QLatin1String("0"));
    CHECK(!genres.isEmpty() && !genres.first().name.isEmpty());

    bool hasRussian = false;
    for (const ChartSource &country : appleCountries())
        hasRussian = hasRussian || country.id == QLatin1String("ru");
    CHECK(hasRussian);

    bool hasNewMusic = false;
    for (const ChartSource &feed : appleFeeds())
        hasNewMusic = hasNewMusic || feed.id == QLatin1String("new-music");
    CHECK(hasNewMusic);
    CHECK(!soundCloudGenres().isEmpty());

    // --- 5. Запрос для поиска и длительность --------------------------------
    std::printf("\nЗапрос и длительность:\n");
    {
        ChartTrack track;
        track.title = QStringLiteral("One More Time");
        track.artist = QStringLiteral("Daft Punk");
        CHECK(track.searchQuery() == QLatin1String("Daft Punk - One More Time"));

        // «feat. …» мешает поиску: обрезаем хвост.
        track.title = QStringLiteral("Harder Better Faster (feat. Nobody)");
        CHECK(track.searchQuery() == QLatin1String("Daft Punk - Harder Better Faster"));

        ChartTrack withoutArtist;
        withoutArtist.title = QStringLiteral("Только название");
        CHECK(withoutArtist.searchQuery() == QStringLiteral("Только название"));

        ChartTrack time;
        time.durationMs = 0;
        CHECK(time.durationText() == QLatin1String("--:--"));
        time.durationMs = 3000;
        CHECK(time.durationText() == QLatin1String("0:03"));
        time.durationMs = 63000;
        CHECK(time.durationText() == QLatin1String("1:03"));
        time.durationMs = 3723000;
        CHECK(time.durationText() == QLatin1String("1:02:03"));
    }

    // --- 6. Запись yt-dlp → строка чарта ------------------------------------
    std::printf("\nЗаписи yt-dlp → строки чарта:\n");
    {
        YtDlpResult result;
        result.title = QStringLiteral("Chart Hit");
        result.uploader = QStringLiteral("Chart Artist");
        result.webpageUrl = QStringLiteral("https://soundcloud.com/chart/hit");
        result.thumbnailUrl = QStringLiteral("https://soundcloud.com/img/hit.jpg");
        result.durationSec = 185;

        const QList<ChartTrack> tracks = chartTracksFromYtDlp({result});
        CHECK(tracks.size() == 1);
        if (!tracks.isEmpty()) {
            CHECK(tracks.first().title == QLatin1String("Chart Hit"));
            CHECK(tracks.first().artist == QLatin1String("Chart Artist"));
            CHECK(tracks.first().sourceName() == QLatin1String("SoundCloud"));
            CHECK(tracks.first().durationMs == 185000);
            CHECK(tracks.first().coverUrl.endsWith(QLatin1String("hit.jpg")));
            CHECK(tracks.first().searchQuery() == QLatin1String("Chart Artist - Chart Hit"));
        }

        // Запись без названия в полку не попадает.
        CHECK(chartTracksFromYtDlp({YtDlpResult{}}).isEmpty());
    }

    // --- 7. Чарт SoundCloud через yt-dlp (заглушка) -------------------------
    std::printf("\nЧарт SoundCloud через yt-dlp:\n");
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QList<YtDlpResult> results;
        bool failed = false;
        QString failure;

        QObject::connect(&ytdlp, &YtDlp::searchFinished,
                         [&results](const QList<YtDlpResult> &list) { results = list; });
        QObject::connect(&ytdlp, &YtDlp::searchFailed, [&](const QString &error) {
            failed = true;
            failure = error;
        });

        ytdlp.browse(soundCloudChartUrl(QStringLiteral("all-music")), 30);

        CHECK(waitFor([&]() { return !results.isEmpty() || failed; }));
        CHECK(!failed);
        CHECK(failure.isEmpty());
        CHECK(results.size() == 3);

        if (results.size() == 3) {
            CHECK(results.at(0).title == QLatin1String("Chart One"));
            CHECK(results.at(0).uploader == QLatin1String("Chart Artist"));
            CHECK(results.at(0).durationSec == 181);
            CHECK(results.at(0).thumbnailUrl.endsWith(QLatin1String("one.jpg")));
            // channel тоже считается исполнителем.
            CHECK(results.at(1).uploader == QLatin1String("Second Artist"));
            // Без длительности (live/неизвестно) — не выдумываем время.
            CHECK(results.at(1).durationSec == 0);
            CHECK(results.at(2).webpageUrl == QLatin1String("https://soundcloud.com/chart/three"));
        }

        const QList<ChartTrack> chart = chartTracksFromYtDlp(results);
        CHECK(chart.size() == 3);
        CHECK(chart.isEmpty() ? false : chart.first().source == QLatin1String("soundcloud"));
        CHECK(!ytdlp.isBusy());
    }

    // --- 8. Ошибка чтения чарта --------------------------------------------
    std::printf("\nОшибка чтения чарта:\n");
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        bool failed = false;
        QString failure;
        QObject::connect(&ytdlp, &YtDlp::searchFailed, [&](const QString &error) {
            failed = true;
            failure = error;
        });

        ytdlp.browse(QStringLiteral("https://fail.test/charts/top"), 5);
        CHECK(waitFor([&]() { return failed; }));
        CHECK(!failure.isEmpty());
        CHECK(!ytdlp.isBusy());
    }

    std::printf("\n%s (провалов: %d)\n", g_failures == 0 ? "ЧАРТЫ: ОК" : "ЧАРТЫ: ЕСТЬ ОШИБКИ",
                g_failures);
    return g_failures == 0 ? 0 : 1;
}