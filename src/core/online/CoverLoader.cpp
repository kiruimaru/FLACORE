#include "CoverLoader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace {

constexpr int kMaxCoverSide = 240;   // в чартах хватает миниатюры

} // namespace

CoverLoader::CoverLoader(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString CoverLoader::cacheDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir = QDir(base.isEmpty() ? QDir::tempPath() : base)
                            .filePath(QStringLiteral("covercache"));
    QDir().mkpath(dir);
    return dir;
}

QString CoverLoader::cacheFileName(const QString &url)
{
    const QByteArray hash
        = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(hash) + QStringLiteral(".img");
}

QPixmap CoverLoader::cached(const QString &url) const
{
    if (url.isEmpty())
        return QPixmap();

    const auto inMemory = m_memory.constFind(url);
    if (inMemory != m_memory.constEnd())
        return inMemory.value();

    if (m_missing.contains(url))
        return QPixmap();

    const QString path = QDir(cacheDir()).filePath(cacheFileName(url));
    if (!QFile::exists(path)) {
        m_missing.insert(url);
        return QPixmap();
    }

    QPixmap pixmap;
    if (!pixmap.load(path)) {
        m_missing.insert(url);
        return QPixmap();
    }

    pixmap = pixmap.scaled(kMaxCoverSide, kMaxCoverSide, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
    m_memory.insert(url, pixmap);
    return pixmap;
}

void CoverLoader::request(const QString &url)
{
    if (url.isEmpty() || m_pending.contains(url))
        return;
    if (!cached(url).isNull())
        return;

    m_missing.remove(url);
    m_pending.insert(url);

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FLACORE/0.4 (Qt)"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_pending.remove(url);
        if (reply->error() != QNetworkReply::NoError) {
            m_missing.insert(url);   // не будем долбить битую ссылку повторно
            return;
        }
        handleFinished(url, reply->readAll());
    });
}

void CoverLoader::handleFinished(const QString &url, const QByteArray &data)
{
    QPixmap pixmap;
    if (!pixmap.loadFromData(data) || pixmap.isNull()) {
        m_missing.insert(url);
        return;
    }

    pixmap = pixmap.scaled(kMaxCoverSide, kMaxCoverSide, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);

    const QString path = QDir(cacheDir()).filePath(cacheFileName(url));
    pixmap.save(path, "PNG");          // лучший вариант для повторного запуска

    m_memory.insert(url, pixmap);
    emit loaded(url);
}