#ifndef YTDLPSETTINGSFORM_H
#define YTDLPSETTINGSFORM_H

#include <QHash>
#include <QString>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;

/**
 * Форма настроек yt-dlp. Вынесена из диалога, чтобы её же показывать отдельной
 * страницей в сайдбаре (и не держать две копии логики).
 */
class YtDlpSettingsForm : public QWidget
{
    Q_OBJECT

public:
    explicit YtDlpSettingsForm(QWidget *parent = nullptr);

    QString binaryPath() const;
    QString ffmpegDir() const;
    QString libraryDir() const;
    QString audioFormat() const;

    void setBinaryPath(const QString &path);

signals:
    void changed();

private:
    void browseBinary();
    void browseFfmpeg();
    void browseLibrary();
    void detectYtDlp();
    void updateHint();
    QString measureBinary() const;

    QLineEdit *m_binaryEdit = nullptr;
    QLineEdit *m_ffmpegEdit = nullptr;
    QLineEdit *m_libraryEdit = nullptr;
    QComboBox *m_formatCombo = nullptr;
    QLabel *m_hint = nullptr;
    mutable QHash<QString, QString> m_versionCache;
};

#endif // YTDLPSETTINGSFORM_H