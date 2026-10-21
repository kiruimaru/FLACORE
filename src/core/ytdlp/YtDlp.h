#ifndef YTDLP_H
#define YTDLP_H

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>

#include "YtDlpTypes.h"

class QProcess;

/**
 * Асинхронная обёртка над CLI yt-dlp.
 *
 * Один экземпляр = один процесс: одновременно выполняется не более одной
 * операции (проверка, поиск или скачивание). UI должен блокировать кнопки,
 * пока isBusy() == true.
 *
 * Каталоги по умолчанию:
 *   библиотека  ~/Music/FLACORE            (QSettings: ytdlp/libraryDir)
 *   кэш/стриминг <CacheLocation>/ytdlp     (скачал -> отдал путь -> играем)
 *
 * Пути к бинарникам:
 *   yt-dlp  QSettings: ytdlp/binary   -> иначе QStandardPaths::findExecutable
 *   ffmpeg  QSettings: ytdlp/ffmpegDir-> нужен только для конвертации в flac/mp3
 */
class YtDlp : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        Idle,
        Checking,
        Searching,
        Downloading
    };

    /** Где искать. Прямые ссылки определяются автоматически. */
    enum class Provider {
        YouTube,
        SoundCloud
    };

    explicit YtDlp(QObject *parent = nullptr);
    ~YtDlp() override;

    // --- конфигурация / пути ------------------------------------------------
    static QString resolveBinary();
    static QString resolveFfmpegDir();
    static QString libraryDir();   // создаёт каталог при необходимости
    static QString cacheDir();     // создаёт каталог при необходимости

    QString binary() const { return m_binary; }
    void setBinary(const QString &path);

    Mode mode() const { return m_mode; }
    bool isBusy() const { return m_mode != Mode::Idle; }

    // --- операции -----------------------------------------------------------
    /** Запускает `yt-dlp --version` -> availabilityChecked(). */
    void checkAvailability();

    /**
     * Поиск. Если query начинается с http:// или https:// — yt-dlp получает
     * метаданные этого URL (одна запись). Иначе выполняется ytsearch/scsearch.
     */
    void search(const QString &query, int limit = 15,
                Provider provider = Provider::YouTube);

    /**
     * Скачивание.
     * @param url          адрес видео/трека
     * @param outputDir    каталог назначения
     * @param audioFormat  пустая строка = скачать bestaudio как есть (для
     *                     прослушивания в приложении, без участия ffmpeg);
     *                     "flac"/"mp3"/"opus" = извлечь аудио и перекодировать
     *                     (нужен бинарник ffmpeg и метаданные).
     */
    void download(const QString &url, const QString &outputDir,
                  const QString &audioFormat = QString(), bool embedMetadata = true);

    /** Прерывает текущую операцию; придёт сигнал cancelled(). */
    void cancel();

signals:
    void availabilityChecked(bool available, const QString &versionOrError);
    void searchFinished(const QList<YtDlpResult> &results);
    void searchFailed(const QString &error);

    void downloadProgress(int percent, const QString &speed, const QString &eta,
                          qint64 downloadedBytes, qint64 totalBytes);
    void downloadStatus(const QString &status);
    void downloadFinished(const QString &filePath);
    void downloadFailed(const QString &error);

    void cancelled();

private:
    void startProcess(const QStringList &args);
    void drainLines();
    void handleLine(const QString &line);
    void parseJson(const QByteArray &json);
    void finishProcess();
    void fail(const QString &message);

    void onReadyRead();
    void onFinished(int exitCode, int exitStatus);
    void onErrorOccurred(int error);

    QProcess *m_process = nullptr;
    Mode m_mode = Mode::Idle;
    QString m_binary;
    QString m_stdoutBuffer;
    QString m_stderrTail;
    QString m_lastFilePath;
    bool m_cancelRequested = false;
};

#endif // YTDLP_H
