#ifndef YTDLPSETTINGSDIALOG_H
#define YTDLPSETTINGSDIALOG_H

#include <QDialog>
#include <QHash>
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;

/**
 * Настройки интеграции yt-dlp: пути к бинарникам, папка библиотеки, формат
 * сохранения. Значения хранятся в QSettings (ключи ytdlp/… ) и применяются
 * вызывающей стороной через YtDlp::setBinary().
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

#endif // YTDLPSETTINGSDIALOG_H
