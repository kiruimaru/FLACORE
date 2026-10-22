#ifndef YTDLPPAGE_H
#define YTDLPPAGE_H

#include <QWidget>

class QLabel;

class YtDlpSettingsForm;

/**
 * Страница «yt-dlp» в сайдбаре: те же поля, что в диалоге, плюс кнопки
 * сохранения и проверки бинарника.
 */
class YtDlpPage : public QWidget
{
    Q_OBJECT

public:
    explicit YtDlpPage(QWidget *parent = nullptr);

    YtDlpSettingsForm *form() const { return m_form; }
    void showStatus(const QString &text, bool error = false);

signals:
    /// Настройки применены и сохранены в QSettings.
    void applied(const QString &binaryPath);
    void checkRequested();

private:
    void save();

    YtDlpSettingsForm *m_form = nullptr;
    QLabel *m_status = nullptr;
};

#endif // YTDLPPAGE_H