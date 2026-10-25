#include "Charts.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

QString cleaned(const QString &text)
{
    return text.simplified();
}

qint64 secondsToMs(const QJsonValue &value)
{
    if (value.isDouble()) {
        const double seconds = value.toDouble();
        if (seconds > 0)
            return qint64(seconds * 1000.0);
    }
    // Apple отдаёт длительность не всегда; Deezer — в секундах.
    return 0;
}

/// Apple отдаёт 100x100, но тот же файл доступен в 600x600 заменой размера.
QString biggerAppleArtwork(const QString &url)
{
    if (url.isEmpty())
        return QString();
    QString result = url;
    result.replace(QStringLiteral("100x100bb"), QStringLiteral("600x600bb"));
    return result;
}

} // namespace

QString ChartTrack::searchQuery() const
{
    if (artist.isEmpty())
        return title;

    QString query = artist + QStringLiteral(" - ") + title;
    // «feat. …» в названиях мешает поиску — оставляем основной вариант.
    const int featIndex = query.indexOf(QStringLiteral(" (feat"), 0, Qt::CaseInsensitive);
    if (featIndex > 0)
        query = query.left(featIndex);
    return query.simplified();
}

QString ChartTrack::durationText() const
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

QString ChartTrack::sourceName() const
{
    if (source == QLatin1String("deezer"))
        return QStringLiteral("Deezer");
    if (source == QLatin1String("apple"))
        return QStringLiteral("Apple Music");
    if (source == QLatin1String("soundcloud"))
        return QStringLiteral("SoundCloud");
    return source;
}

QList<ChartSource> deezerGenres()
{
    return {
        {QStringLiteral("0"), QStringLiteral("Топ — весь мир")},
        {QStringLiteral("132"), QStringLiteral("Поп")},
        {QStringLiteral("116"), QStringLiteral("Rap / Hip Hop")},
        {QStringLiteral("106"), QStringLiteral("Электронная")},
        {QStringLiteral("113"), QStringLiteral("Танцевальная")},
        {QStringLiteral("152"), QStringLiteral("Рок")},
        {QStringLiteral("85"), QStringLiteral("Альтернативная")},
        {QStringLiteral("165"), QStringLiteral("R&B")},
        {QStringLiteral("169"), QStringLiteral("Soul & Funk")},
        {QStringLiteral("129"), QStringLiteral("Джаз")},
        {QStringLiteral("153"), QStringLiteral("Блюз")},
        {QStringLiteral("98"), QStringLiteral("Классика")},
        {QStringLiteral("464"), QStringLiteral("Металл")},
        {QStringLiteral("466"), QStringLiteral("Фолк")},
        {QStringLiteral("144"), QStringLiteral("Регги")},
        {QStringLiteral("197"), QStringLiteral("Латино")},
        {QStringLiteral("544"), QStringLiteral("Русская музыка")},
    };
}

QList<ChartSource> appleCountries()
{
    return {
        {QStringLiteral("us"), QStringLiteral("США")},
        {QStringLiteral("gb"), QStringLiteral("Великобритания")},
        {QStringLiteral("de"), QStringLiteral("Германия")},
        {QStringLiteral("fr"), QStringLiteral("Франция")},
        {QStringLiteral("ru"), QStringLiteral("Россия")},
    };
}

QList<ChartSource> appleFeeds()
{
    return {
        {QStringLiteral("most-played"), QStringLiteral("Самые прослушиваемые")},
        {QStringLiteral("new-music"), QStringLiteral("Новинки")},
    };
}

QList<ChartSource> soundCloudGenres()
{
    return {
        {QStringLiteral("all-music"), QStringLiteral("Всё")},
        {QStringLiteral("electronic"), QStringLiteral("Электронная")},
        {QStringLiteral("house"), QStringLiteral("House")},
        {QStringLiteral("techno"), QStringLiteral("Techno")},
        {QStringLiteral("hiphoprap"), QStringLiteral("Hip-hop / Rap")},
        {QStringLiteral("pop"), QStringLiteral("Поп")},
        {QStringLiteral("rock"), QStringLiteral("Рок")},
        {QStringLiteral("ambient"), QStringLiteral("Ambient")},
        {QStringLiteral("jazzblues"), QStringLiteral("Jazz & Blues")},
        {QStringLiteral("triphop"), QStringLiteral("Trip-hop")},
        {QStringLiteral("experimental"), QStringLiteral("Экспериментальная")},
    };
}

QString soundCloudChartUrl(const QString &genreSlug)
{
    const QString slug = genreSlug.trimmed().isEmpty() ? QStringLiteral("all-music")
                                                       : genreSlug.trimmed();
    // Используем on.soundcloud.com — yt-dlp корректно разбирает именно этот хост.
    // soundcloud.com/charts вызывал ошибку парсинга (Issue #8).
    return QStringLiteral("https://on.soundcloud.com/charts/top?genre=%1").arg(slug);
}

QList<ChartTrack> chartTracksFromYtDlp(const QList<YtDlpResult> &results)
{
    QList<ChartTrack> tracks;
    tracks.reserve(results.size());

    for (const YtDlpResult &result : results) {
        ChartTrack track;
        track.title = result.title;
        track.artist = result.uploader;
        track.source = QStringLiteral("soundcloud");
        track.url = result.webpageUrl;
        track.coverUrl = result.thumbnailUrl;
        track.durationMs = result.durationSec * 1000;
        if (!track.title.isEmpty())
            tracks.append(track);
    }

    return tracks;
}

// --- парсеры ----------------------------------------------------------------

QList<ChartTrack> parseDeezerChart(const QByteArray &json, QString *error)
{
    QList<ChartTrack> tracks;

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (error)
            *error = QObject::tr("Ответ Deezer не разобран: %1").arg(parseError.errorString());
        return tracks;
    }

    const QJsonObject root = doc.object();
    // Ошибка Deezer: {"error":{"type":"...","message":"..."}}
    if (root.contains(QStringLiteral("error"))) {
        const QJsonObject err = root.value(QStringLiteral("error")).toObject();
        if (error)
            *error = QObject::tr("Deezer: %1").arg(err.value(QStringLiteral("message")).toString());
        return tracks;
    }

    const QJsonArray data = root.value(QStringLiteral("data")).toArray();
    tracks.reserve(data.size());
    for (const QJsonValue &value : data) {
        const QJsonObject entry = value.toObject();
        const QJsonObject artist = entry.value(QStringLiteral("artist")).toObject();

        ChartTrack track;
        track.title = cleaned(entry.value(QStringLiteral("title")).toString());
        track.artist = cleaned(artist.value(QStringLiteral("name")).toString());
        track.album = cleaned(entry.value(QStringLiteral("album")).toObject()
                                  .value(QStringLiteral("title"))
                                  .toString());
        track.source = QStringLiteral("deezer");
        track.url = entry.value(QStringLiteral("link")).toString();
        track.previewUrl = entry.value(QStringLiteral("preview")).toString();
        track.coverUrl = entry.value(QStringLiteral("album")).toObject()
                             .value(QStringLiteral("cover_medium"))
                             .toString();
        if (track.coverUrl.isEmpty()) {
            track.coverUrl = entry.value(QStringLiteral("artist")).toObject()
                                 .value(QStringLiteral("picture_medium"))
                                 .toString();
        }
        track.durationMs = secondsToMs(entry.value(QStringLiteral("duration")));

        if (!track.title.isEmpty())
            tracks.append(track);
    }

    return tracks;
}

QList<ChartTrack> parseAppleChart(const QByteArray &json, QString *error)
{
    QList<ChartTrack> tracks;

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (error)
            *error = QObject::tr("Ответ Apple не разобран: %1").arg(parseError.errorString());
        return tracks;
    }

    const QJsonObject feed = doc.object().value(QStringLiteral("feed")).toObject();
    const QJsonArray results = feed.value(QStringLiteral("results")).toArray();
    if (results.isEmpty()) {
        if (error)
            *error = QObject::tr("Apple вернул пустой список");
        return tracks;
    }

    tracks.reserve(results.size());
    for (const QJsonValue &value : results) {
        const QJsonObject entry = value.toObject();

        ChartTrack track;
        track.title = cleaned(entry.value(QStringLiteral("name")).toString());
        track.artist = cleaned(entry.value(QStringLiteral("artistName")).toString());
        track.album = cleaned(entry.value(QStringLiteral("collectionName")).toString());
        track.source = QStringLiteral("apple");
        track.url = entry.value(QStringLiteral("url")).toString();
        track.previewUrl = entry.value(QStringLiteral("previewUrl")).toString();
        track.coverUrl = biggerAppleArtwork(entry.value(QStringLiteral("artworkUrl100")).toString());
        // Apple отдаёт durationInMillis; если его нет — пробуем поле duration.
        const QJsonValue durationMs = entry.value(QStringLiteral("durationInMillis"));
        if (durationMs.isDouble())
            track.durationMs = qint64(durationMs.toDouble());
        else
            track.durationMs = secondsToMs(entry.value(QStringLiteral("duration")));

        if (!track.title.isEmpty())
            tracks.append(track);
    }

    return tracks;
}

// --- загрузчик --------------------------------------------------------------

ChartsFetcher::ChartsFetcher(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString ChartsFetcher::deezerUrl(const QString &genreId, int limit)
{
    const QString id = genreId.trimmed().isEmpty() ? QStringLiteral("0") : genreId.trimmed();
    return QStringLiteral("https://api.deezer.com/chart/%1/tracks?limit=%2")
        .arg(id)
        .arg(qBound(1, limit, 100));
}

QString ChartsFetcher::appleUrl(const QString &feed, const QString &country, int limit)
{
    return QStringLiteral("https://rss.applemarketingtools.com/api/v2/%1/music/%2/%3/songs.json")
        .arg(country.isEmpty() ? QStringLiteral("us") : country)
        .arg(feed.isEmpty() ? QStringLiteral("most-played") : feed)
        .arg(qBound(1, limit, 100));
}

void ChartsFetcher::fetchDeezer(const QString &genreId, int limit)
{
    get(deezerUrl(genreId, limit), true);
}

void ChartsFetcher::fetchApple(const QString &feed, const QString &country, int limit)
{
    get(appleUrl(feed, country, limit), false);
}

void ChartsFetcher::get(const QString &url, bool deezer)
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("FLACORE/0.4 (Qt)"));
    // Apple отдаёт 301 на новый хост: редиректы нужно проходить.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, deezer]() {
        handleReply(reply, deezer);
    });
}

void ChartsFetcher::handleReply(QNetworkReply *reply, bool deezer)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(tr("%1 недоступен: %2")
                        .arg(deezer ? QStringLiteral("Deezer") : QStringLiteral("Apple Music"),
                             reply->errorString()),
                    deezer);
        return;
    }

    const QByteArray payload = reply->readAll();
    QString error;
    const QList<ChartTrack> tracks = deezer ? parseDeezerChart(payload, &error)
                                            : parseAppleChart(payload, &error);

    if (tracks.isEmpty()) {
        emit failed(error.isEmpty() ? tr("Пустой ответ от источника") : error, deezer);
        return;
    }

    if (deezer)
        emit deezerReady(tracks);
    else
        emit appleReady(tracks);
}