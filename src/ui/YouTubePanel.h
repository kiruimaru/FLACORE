#ifndef YOUTUBEPANEL_H
#define YOUTUBEPANEL_H

#include <QWidget>

#include "core/ytdlp/YtDlpTypes.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QListWidgetItem;

class YtDlp;

/**
 * Вкладка «YouTube»: поиск/вставка ссылки, прослушивание и сохранение
 * в библиотеку через yt-dlp.
 *
 * Вкладка не знает о QMediaPlayer и Playlist — она только отдаёт наружу
 * готовый локальный файл сигналами playRequested()/fileAdded().
 */
class YouTubePanel : public QWidget
{
    Q_OBJECT

public:
    explicit YouTubePanel(YtDlp *ytdlp, QWidget *parent = nullptr);

    /**
     * Найти запрос и сразу выполнить действие с первым результатом.
     * Нужно разделу «Обзор»: строка чарта превращается в панель YouTube,
     * где виден прогресс, а плеер получает готовый файл обычным путём.
     *
     * provider < 0 — оставить текущий источник и, если ничего не нашлось,
     * попробовать второй (YouTube ↔ SoundCloud).
     */
    void runQuery(const QString &query, int provider, bool saveToLibrary);

signals:
    /** Скачано для прослушивания: можно сразу играть. */
    void playRequested(const QString &filePath);
    /** Скачано в библиотеку: добавить в плейлист. */
    void fileAdded(const QString &filePath);

private:
    enum class Job {
        None,
        PlayNow,
        SaveToLibrary
    };

    void setupUi();
    void runSearch();
    /// Попробовать второй источник, если в первом ничего не нашлось.
    bool tryFallbackProvider();
    void startJob(Job job);
    void updateUi();
    void setStatus(const QString &text);
    void setError(const QString &text);

    bool selectedTarget(QString *url, QString *title) const;
    void addResultItem(const YtDlpResult &result);
    /// Подсказка в списке результатов, пока/если результатов нет.
    void showResultsHint(const QString &text);
    /// Следующий подходящий результат поиска (перебор вариантов из «Обзора»).
    bool selectNextResult();
    /// Запомнить источник, который сработал: следующий поиск начнётся с него.
    void rememberProvider();

    YtDlp *m_ytdlp = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_sourceCombo = nullptr;
    QPushButton *m_searchButton = nullptr;
    QListWidget *m_results = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_status = nullptr;

    Job m_job = Job::None;
    /// Работа, которую нужно запустить сразу после поиска (для «Обзора»).
    Job m_pendingJob = Job::None;
    /// Задание пришло из «Обзора»: если трек не скачался (например, DRM),
    /// имеет смысл попробовать следующую находку.
    bool m_autoJob = false;
    Job m_autoJobKind = Job::None;
    int m_autoAttempts = 0;
    /// Поиск запущен этой панелью: чужие результаты (чарты) не подставляем.
    bool m_searchPending = false;
    /// Второй источник уже пробовали — не зацикливаться.
    bool m_fallbackTried = false;
};

#endif // YOUTUBEPANEL_H