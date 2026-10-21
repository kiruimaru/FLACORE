#include "YtDlp.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>

namespace {
const QLatin1String kProgressMarker("FLACORE_PROGRESS ");
const QLatin1String kFileMarker("FLACORE_FILE ");

// Шаблон прогресса: парсим одной строкой, префикс нужен, чтобы не путать с
// обычным выводом yt-dlp.
const char *kProgressTemplate =
    "download:FLACORE_PROGRESS %(progress._percent_str)s|"
    "%(progress.downloaded_bytes)s|"
    "%(progress.total_bytes_estimate)s|"
    "%(progress._speed_str)s|"
    "%(progress._eta_str)s";

QString lastMeaningfulLine(const QString &text)
{
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        QString cleaned = line;
        if (cleaned.startsWith(QLatin1String("ERROR:")))
            cleaned = cleaned.mid(6).trimmed();
        return cleaned;
    }
    return QString();
}
} // namespace

YtDlp::YtDlp(QObject *parent)
    : QObject(parent)
    , m_binary(resolveBinary())
{
}

YtDlp::~YtDlp()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

// ---------------------------------------------------------------------------
// Пути и настройки
// ---------------------------------------------------------------------------

QString YtDlp::resolveBinary()
{
    const QString configured = QSettings().value(QStringLiteral("ytdlp/binary")).toString().trimmed();
    if (!configured.isEmpty())
        return configured;
    return QStandardPaths::findExecutable(QStringLiteral("yt-dlp"));
}

QString YtDlp::resolveFfmpegDir()
{
    const QString configured = QSettings().value(QStringLiteral("ytdlp/ffmpegDir")).toString().trimmed();
    if (!configured.isEmpty())
        return configured;

    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (!ffmpeg.isEmpty())
        return QFileInfo(ffmpeg).absolutePath();
    return QString();
}

QString YtDlp::libraryDir()
{
    QString dir = QSettings().value(QStringLiteral("ytdlp/libraryDir")).toString().trimmed();
    if (dir.isEmpty()) {
        const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        dir = QDir(music.isEmpty() ? QDir::homePath() : music).filePath(QStringLiteral("FLACORE"));
    }
    QDir().mkpath(dir);
    return dir;
}

QString YtDlp::cacheDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString dir = QDir(base.isEmpty() ? QDir::tempPath() : base).filePath(QStringLiteral("ytdlp"));
    QDir().mkpath(dir);
    return dir;
}

void YtDlp::setBinary(const QString &path)
{
    m_binary = path.trimmed();
    QSettings().setValue(QStringLiteral("ytdlp/binary"), m_binary);
}

// ---------------------------------------------------------------------------
// Публичные операции
// ---------------------------------------------------------------------------

void YtDlp::checkAvailability()
{
    if (isBusy())
        return;
    m_mode = Mode::Checking;
    startProcess(QStringList{QStringLiteral("--version")});
}

void YtDlp::search(const QString &query, int limit, Provider provider)
{
    const QString text = query.trimmed();
    if (text.isEmpty() || isBusy())
        return;

    QStringList args;
    if (text.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        || text.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        args << text << QStringLiteral("--no-playlist");
    } else {
        const QString prefix = (provider == Provider::SoundCloud)
                                   ? QStringLiteral("scsearch")
                                   : QStringLiteral("ytsearch");
        args << QStringLiteral("%1%2:%3").arg(prefix).arg(qBound(1, limit, 50)).arg(text);
    }

    args << QStringLiteral("--dump-single-json")
         << QStringLiteral("--flat-playlist")
         << QStringLiteral("--skip-download")
         << QStringLiteral("--no-warnings");

    m_mode = Mode::Searching;
    startProcess(args);
}

void YtDlp::download(const QString &url, const QString &outputDir,
                     const QString &audioFormat, bool embedMetadata)
{
    if (url.trimmed().isEmpty() || isBusy())
        return;

    QDir().mkpath(outputDir);

    QStringList args;
    if (audioFormat.isEmpty()) {
        // Для прослушивания: без перекодирования, m4a предпочтительнее —
        // QMediaPlayer воспроизводит его без ffmpeg.
        args << QStringLiteral("-f") << QStringLiteral("bestaudio[ext=m4a]/bestaudio/best");
    } else {
        args << QStringLiteral("-f") << QStringLiteral("bestaudio/best")
             << QStringLiteral("-x")
             << QStringLiteral("--audio-format") << audioFormat
             << QStringLiteral("--audio-quality") << QStringLiteral("0");
        if (embedMetadata)
            args << QStringLiteral("--embed-metadata") << QStringLiteral("--add-metadata");

        const QString ffmpegDir = resolveFfmpegDir();
        if (!ffmpegDir.isEmpty())
            args << QStringLiteral("--ffmpeg-location") << ffmpegDir;
    }

    args << QStringLiteral("--no-playlist")
         << QStringLiteral("--newline")
         << QStringLiteral("--no-warnings")
         << QStringLiteral("--progress")                      // --print сам по себе глушит прогресс
         << QStringLiteral("--progress-template") << QString::fromLatin1(kProgressTemplate)
         << QStringLiteral("--print") << QStringLiteral("after_move:FLACORE_FILE %(filepath)s");

    const QString nameTemplate = audioFormat.isEmpty()
        ? QStringLiteral("%(title)s [%(id)s].%(ext)s")
        : QStringLiteral("%(artist,uploader,channel)s - %(title)s.%(ext)s");
    args << QStringLiteral("-o") << QDir(outputDir).filePath(nameTemplate);
    args << url.trimmed();

    m_mode = Mode::Downloading;
    startProcess(args);
}

void YtDlp::cancel()
{
    if (!m_process)
        return;
    m_cancelRequested = true;
    m_process->kill();
}

// ---------------------------------------------------------------------------
// Процесс
// ---------------------------------------------------------------------------

void YtDlp::startProcess(const QStringList &args)
{
    if (m_binary.isEmpty()) {
        fail(tr("yt-dlp не найден. Укажите путь к yt-dlp в меню «Инструменты → Настройки yt-dlp»."));
        return;
    }

    m_stdoutBuffer.clear();
    m_stderrTail.clear();
    m_lastFilePath.clear();
    m_cancelRequested = false;

    m_process = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    m_process->setProcessEnvironment(env);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() { onReadyRead(); });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() { onReadyRead(); });
    connect(m_process, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                onFinished(exitCode, static_cast<int>(status));
            });
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) { onErrorOccurred(static_cast<int>(error)); });

    m_process->start(m_binary, args);
}

void YtDlp::onReadyRead()
{
    if (!m_process)
        return;

    m_stdoutBuffer += QString::fromUtf8(m_process->readAllStandardOutput());

    const QByteArray err = m_process->readAllStandardError();
    if (!err.isEmpty()) {
        m_stderrTail += QString::fromUtf8(err);
        if (m_stderrTail.size() > 4000)
            m_stderrTail = m_stderrTail.right(4000);
    }

    if (m_mode == Mode::Downloading)
        drainLines();
}

void YtDlp::drainLines()
{
    for (;;) {
        const int newline = m_stdoutBuffer.indexOf(QLatin1Char('\n'));
        const int carriage = m_stdoutBuffer.indexOf(QLatin1Char('\r'));
        int cut = -1;
        if (newline >= 0 && carriage >= 0)
            cut = qMin(newline, carriage);
        else
            cut = qMax(newline, carriage);

        if (cut < 0)
            return;

        const QString line = m_stdoutBuffer.left(cut);
        m_stdoutBuffer.remove(0, cut + 1);
        if (!line.trimmed().isEmpty())
            handleLine(line.trimmed());
    }
}

void YtDlp::handleLine(const QString &line)
{
    if (line.startsWith(kProgressMarker)) {
        const QString payload = line.mid(kProgressMarker.size());
        const QStringList parts = payload.split(QLatin1Char('|'));
        if (parts.size() < 5)
            return;

        bool ok = false;
        const double percent = parts.at(0).trimmed().remove(QLatin1Char('%')).toDouble(&ok);
        const qint64 done = parts.at(1).trimmed().toLongLong();
        const qint64 total = parts.at(2).trimmed().toLongLong();
        const QString speed = parts.at(3).trimmed();
        const QString eta = parts.at(4).trimmed();

        emit downloadProgress(ok ? qRound(percent) : -1,
                              speed == QLatin1String("NA") ? QString() : speed,
                              eta == QLatin1String("NA") ? QString() : eta,
                              done, qMax<qint64>(0, total));
    } else if (line.startsWith(kFileMarker)) {
        m_lastFilePath = line.mid(kFileMarker.size()).trimmed();
    } else {
        emit downloadStatus(line);
    }
}

void YtDlp::onFinished(int exitCode, int exitStatus)
{
    if (!m_process)
        return;

    m_stdoutBuffer += QString::fromUtf8(m_process->readAllStandardOutput());
    const QByteArray err = m_process->readAllStandardError();
    if (!err.isEmpty()) {
        m_stderrTail += QString::fromUtf8(err);
        if (m_stderrTail.size() > 4000)
            m_stderrTail = m_stderrTail.right(4000);
    }

    // Последняя строка без перевода строки (например, FLACORE_FILE в конце).
    if (m_mode == Mode::Downloading) {
        drainLines();
        const QString tail = m_stdoutBuffer.trimmed();
        m_stdoutBuffer.clear();
        if (!tail.isEmpty())
            handleLine(tail);
    }

    const Mode mode = m_mode;
    const QString output = m_stdoutBuffer.trimmed();
    const QString errorText = m_stderrTail.trimmed();
    const bool normalExit = (exitStatus == static_cast<int>(QProcess::NormalExit));

    // Обнуляем состояние до эмита сигналов, чтобы обработчики могли сразу
    // запускать следующую операцию.
    m_mode = Mode::Idle;
    QProcess *process = m_process;
    m_process = nullptr;
    if (process) {
        process->disconnect(this);
        process->deleteLater();
    }
    m_stdoutBuffer.clear();

    if (m_cancelRequested) {
        m_cancelRequested = false;
        emit cancelled();
        return;
    }

    switch (mode) {
    case Mode::Checking: {
        const bool ok = normalExit && exitCode == 0 && !output.isEmpty();
        emit availabilityChecked(ok, ok ? output.split(QLatin1Char('\n')).value(0).trimmed()
                                        : lastMeaningfulLine(errorText));
        break;
    }
    case Mode::Searching: {
        if (!normalExit || exitCode != 0 || output.isEmpty() || output == QLatin1String("null")) {
            emit searchFailed(lastMeaningfulLine(errorText).isEmpty()
                                  ? tr("yt-dlp вернул пустой ответ")
                                  : lastMeaningfulLine(errorText));
            break;
        }
        parseJson(output.toUtf8());
        break;
    }
    case Mode::Downloading: {
        if (!normalExit || exitCode != 0) {
            emit downloadFailed(lastMeaningfulLine(errorText).isEmpty()
                                    ? tr("yt-dlp завершился с кодом %1").arg(exitCode)
                                    : lastMeaningfulLine(errorText));
            break;
        }
        if (m_lastFilePath.isEmpty()) {
            emit downloadFailed(tr("Не удалось определить путь к скачанному файлу"));
            break;
        }
        emit downloadFinished(m_lastFilePath);
        break;
    }
    case Mode::Idle:
        break;
    }
}

void YtDlp::onErrorOccurred(int error)
{
    if (error != static_cast<int>(QProcess::FailedToStart))
        return;

    const QString binaryPath = m_binary;
    fail(tr("Не удалось запустить yt-dlp (%1). Проверьте путь в настройках.").arg(binaryPath));
}

void YtDlp::fail(const QString &message)
{
    const Mode mode = m_mode;
    m_mode = Mode::Idle;
    m_cancelRequested = false;

    if (m_process) {
        m_process->disconnect(this);
        m_process->deleteLater();
        m_process = nullptr;
    }

    switch (mode) {
    case Mode::Checking:
        emit availabilityChecked(false, message);
        break;
    case Mode::Searching:
        emit searchFailed(message);
        break;
    case Mode::Downloading:
        emit downloadFailed(message);
        break;
    case Mode::Idle:
        emit downloadFailed(message);
        break;
    }
}

// ---------------------------------------------------------------------------
// Разбор JSON
// ---------------------------------------------------------------------------

void YtDlp::parseJson(const QByteArray &json)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        emit searchFailed(tr("Не удалось разобрать ответ yt-dlp: %1").arg(parseError.errorString()));
        return;
    }

    QList<YtDlpResult> results;
    const auto appendEntry = [&results](const QJsonObject &entry) {
        if (entry.isEmpty())
            return;

        YtDlpResult result;
        result.id = entry.value(QStringLiteral("id")).toString();
        result.title = entry.value(QStringLiteral("title")).toString();
        if (result.title.isEmpty())
            result.title = result.id;

        result.uploader = entry.value(QStringLiteral("uploader")).toString();
        if (result.uploader.isEmpty())
            result.uploader = entry.value(QStringLiteral("channel")).toString();
        if (result.uploader.isEmpty())
            result.uploader = entry.value(QStringLiteral("playlist_uploader")).toString();

        if (!entry.value(QStringLiteral("is_live")).toBool(false)) {
            const double seconds = entry.value(QStringLiteral("duration")).toDouble(0.0);
            if (seconds > 0)
                result.durationSec = static_cast<qint64>(qRound64(seconds));
        }

        QString page = entry.value(QStringLiteral("webpage_url")).toString();
        if (page.isEmpty()) {
            const QString url = entry.value(QStringLiteral("url")).toString();
            if (url.startsWith(QLatin1String("http"), Qt::CaseInsensitive))
                page = url;
            else if (!result.id.isEmpty())
                page = QStringLiteral("https://www.youtube.com/watch?v=") + result.id;
        }
        result.webpageUrl = page;

        const QJsonArray thumbnails = entry.value(QStringLiteral("thumbnails")).toArray();
        if (!thumbnails.isEmpty())
            result.thumbnailUrl = thumbnails.last().toObject().value(QStringLiteral("url")).toString();

        if (!result.webpageUrl.isEmpty())
            results.append(result);
    };

    const QJsonObject root = doc.object();
    const QJsonValue entries = root.value(QStringLiteral("entries"));
    if (entries.isArray()) {
        const QJsonArray array = entries.toArray();
        for (const QJsonValue &value : array)
            appendEntry(value.toObject());
    } else {
        // Прямой URL: yt-dlp отдаёт один объект видео.
        appendEntry(root);
    }

    if (results.isEmpty()) {
        emit searchFailed(tr("Ничего не найдено"));
        return;
    }

    emit searchFinished(results);
}
