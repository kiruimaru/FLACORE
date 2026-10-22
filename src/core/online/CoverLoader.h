#ifndef COVERLOADER_H
#define COVERLOADER_H

#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QString>

class QNetworkAccessManager;

/**
 * Асинхронная загрузка обложек чартов с кэшем на диске.
 *
 * cached() отдаёт картинку сразу (память → диск), request() запускает загрузку
 * и сообщает сигналом loaded(), когда её можно взять из cached(). Так полки
 * чартов не блокируют интерфейс и не качают одно и то же дважды.
 */
class CoverLoader : public QObject
{
    Q_OBJECT

public:
    explicit CoverLoader(QObject *parent = nullptr);

    /// Готовая картинка или пустой QPixmap (без сети).
    QPixmap cached(const QString &url) const;

    /// Запросить загрузку, если её ещё нет в кэше.
    void request(const QString &url);

    static QString cacheDir();
    static QString cacheFileName(const QString &url);

signals:
    void loaded(const QString &url);

private:
    void handleFinished(const QString &url, const QByteArray &data);

    mutable QHash<QString, QPixmap> m_memory;
    mutable QSet<QString> m_missing;   // уже проверяли диск и не нашли
    QSet<QString> m_pending;
    QNetworkAccessManager *m_network = nullptr;
};

#endif // COVERLOADER_H