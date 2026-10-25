#ifndef COVERFETCHDIALOG_H
#define COVERFETCHDIALOG_H

#include <QDialog>
#include <QImage>
#include <QString>
#include <QList>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QProgressBar;
class QNetworkAccessManager;
class QProcess;

#include "core/ytdlp/YtDlpTypes.h"

/**
 * Диалог подтягивания обложки (фото) трека из SoundCloud или YouTube (Issue #14).
 * Ищет подходящий трек на SC/YT, скачивает превью-изображение и вшивает его в файл
 * трека через ffmpeg и локальный кэш.
 */
class CoverFetchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoverFetchDialog(const QString &audioPath, const QString &artist,
                              const QString &title, QWidget *parent = nullptr);
    ~CoverFetchDialog() override;

signals:
    void coverApplied(const QImage &image);

private slots:
    void onSearch();
    void onItemSelectionChanged();
    void onApplyCover();
    void onSearchProcessFinished(int exitCode);

private:
    void startSearchProcess(const QString &query, int provider);
    void downloadPreview(const QString &url);

    QString m_audioPath;
    QString m_artist;
    QString m_title;

    QLineEdit *m_queryEdit = nullptr;
    QComboBox *m_providerCombo = nullptr;
    QPushButton *m_searchBtn = nullptr;
    QListWidget *m_resultsList = nullptr;
    QLabel *m_previewLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_applyBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;

    QProcess *m_process = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QList<YtDlpResult> m_results;
    QImage m_currentImage;
    QString m_currentThumbnailUrl;
};

#endif // COVERFETCHDIALOG_H
