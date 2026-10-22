#ifndef YTDLPSETTINGSDIALOG_H
#define YTDLPSETTINGSDIALOG_H

#include <QDialog>
#include <QString>

class YtDlpSettingsForm;

/**
 * Диалог настроек yt-dlp — тонкая обёртка над YtDlpSettingsForm, чтобы те же
 * поля жили ещё и отдельной страницей в сайдбаре без второй копии логики.
 * Значения хранятся в QSettings (ключи ytdlp/…) и применяются вызывающей
 * стороной через YtDlp::setBinary().
 */
class YtDlpSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit YtDlpSettingsDialog(QWidget *parent = nullptr);

    QString binaryPath() const;
    QString ffmpegDir() const;
    QString libraryDir() const;
    QString audioFormat() const;

private:
    YtDlpSettingsForm *m_form = nullptr;
};

#endif // YTDLPSETTINGSDIALOG_H