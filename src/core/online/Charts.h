#ifndef CHARTS_H
#define CHARTS_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include "core/ytdlp/YtDlpTypes.h"

class QNetworkAccessManager;
class QNetworkReply;

/**
 * «Живая подборка»: чарты из открытых источников без регистрации и ключей.
 *
 * Источники:
 *   Deezer   — api.deezer.com/chart/<жанр>/tracks  (топ по всему миру и по жанрам);
 *   Apple    — rss.applemarketingtools.com/api/v2/<страна>/music/<лента>/N/songs.json
 *              (самые прослушиваемые и новинки по стране);
 *   SoundCloud — чарты идут через yt-dlp (см. DiscoverPage), потому что
 *              публичного API без ключа у них нет.
 *
 * Разбор ответов вынесен в свободные функции: парсеры проверяются тестами на
 * фикстурах и не требуют сети.
 */

struct ChartTrack {
    QString title;
    QString artist;
    QString album;
    QString source;       // "deezer" | "apple" | "soundcloud"
    QString url;          // страница трека (открыть в браузере)
    QString coverUrl;     // картинка обложки, может быть пустой
    QString previewUrl;   // короткий mp3-превью, если источник его даёт
    qint64 durationMs = 0;

    /// Запрос для поиска в yt-dlp, когда трек нужно скачать и проиграть.
    QString searchQuery() const;
    QString durationText() const;
    QString sourceName() const;   // «Deezer», «Apple Music», «SoundCloud»
};

/// Пункт выбора для чарта: id источника + человекочитаемое имя.
struct ChartSource {
    QString id;
    QString name;
};

/// Жанры Deezer (id взяты из api.deezer.com/genre).
QList<ChartSource> deezerGenres();
/// Страны для чартов Apple Music.
QList<ChartSource> appleCountries();
/// Ленты Apple Music.
QList<ChartSource> appleFeeds();
/// Жанры для чарта SoundCloud (значения genre у soundcloud.com/charts).
QList<ChartSource> soundCloudGenres();
/// Ссылка на чарт SoundCloud, которую разбирает yt-dlp.
QString soundCloudChartUrl(const QString &genreSlug);

/**
 * Превратить записи yt-dlp (чарт SoundCloud) в строки чарта.
 * Отдельная функция, потому что её проверяет тест без сети и без UI.
 */
QList<ChartTrack> chartTracksFromYtDlp(const QList<YtDlpResult> &results);

/// Разбор ответа Deezer: {"data":[{"title","artist":{"name"},"duration","preview",...}]}
QList<ChartTrack> parseDeezerChart(const QByteArray &json, QString *error = nullptr);
/// Разбор ответа Apple: {"feed":{"results":[{"name","artistName","artworkUrl100",...}]}}
QList<ChartTrack> parseAppleChart(const QByteArray &json, QString *error = nullptr);

/**
 * Загрузчик чартов: один GET на запрос, результат — сигнал со списком.
 * Ничего не кэширует: обновление делает страница по кнопке.
 */
class ChartsFetcher : public QObject
{
    Q_OBJECT

public:
    explicit ChartsFetcher(QObject *parent = nullptr);

    /// genreId: пусто или "0" — весь мир.
    void fetchDeezer(const QString &genreId = QString(), int limit = 30);
    /// feed: most-played | new-music | top-songs; country: us, gb, de, fr, ru…
    void fetchApple(const QString &feed = QStringLiteral("most-played"),
                    const QString &country = QStringLiteral("us"), int limit = 30);

    static QString deezerUrl(const QString &genreId, int limit);
    static QString appleUrl(const QString &feed, const QString &country, int limit);

signals:
    void deezerReady(const QList<ChartTrack> &tracks);
    void appleReady(const QList<ChartTrack> &tracks);
    /// Ошибка источника: deezer=true — упал Deezer, иначе Apple Music.
    void failed(const QString &error, bool deezer);

private:
    void get(const QString &url, bool deezer);
    void handleReply(QNetworkReply *reply, bool deezer);

    QNetworkAccessManager *m_network = nullptr;
};

#endif // CHARTS_H