#ifndef YTDLP_TYPES_H
#define YTDLP_TYPES_H

#include <QString>
#include <QList>
#include <QMetaType>

/**
 * Один результат поиска yt-dlp (или прямой URL, введённый пользователем).
 */
struct YtDlpResult {
    QString id;
    QString title;
    QString uploader;
    QString webpageUrl;
    QString thumbnailUrl;
    qint64 durationSec = 0;
};

/// Формат длительности: 3:45 или 1:02:03 (минуты без ведущего нуля — как в плеерах).
inline QString formatDuration(qint64 totalSeconds)
{
    if (totalSeconds <= 0)
        return QStringLiteral("--:--");

    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    const auto twoDigits = [](qint64 value) {
        return QString::number(value).rightJustified(2, QLatin1Char('0'));
    };

    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours).arg(twoDigits(minutes), twoDigits(seconds));

    return QStringLiteral("%1:%2").arg(minutes).arg(twoDigits(seconds));
}

Q_DECLARE_METATYPE(YtDlpResult)
Q_DECLARE_METATYPE(QList<YtDlpResult>)

#endif // YTDLP_TYPES_H
