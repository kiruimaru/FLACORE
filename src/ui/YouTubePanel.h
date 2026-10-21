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
    void startJob(Job job);
    void updateUi();
    void setStatus(const QString &text);
    void setError(const QString &text);

    bool selectedTarget(QString *url, QString *title) const;
    void addResultItem(const YtDlpResult &result);

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
};

#endif // YOUTUBEPANEL_H
