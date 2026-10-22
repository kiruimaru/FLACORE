#ifndef APPEARANCEPAGE_H
#define APPEARANCEPAGE_H

#include <QWidget>

#include "ui/theme/Theme.h"

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class QTimer;
class QVBoxLayout;

class GlassPanel;
class ThemeTile;

/**
 * Страница «Оформление»: темы как пакеты (theme.json + theme.qss + assets),
 * акцент, фон, шрифт, стекло/производительность и свой CSS с живым применением.
 * Всё хранится в QSettings, поэтому следующий запуск открывается в том же виде.
 */
class AppearancePage : public QWidget
{
    Q_OBJECT

public:
    explicit AppearancePage(QWidget *parent = nullptr);

    /// Безрамочное окно (своя шапка) — хранится в настройках, читается MainWindow.
    static bool framelessEnabled();
    static void setFramelessEnabled(bool enabled);

signals:
    void framelessToggled(bool enabled);

private:
    void buildThemeCard(QWidget *parent, QVBoxLayout *layout);
    void buildAccentCard(QWidget *parent, QVBoxLayout *layout);
    void buildBackgroundCard(QWidget *parent, QVBoxLayout *layout);
    void buildTypeCard(QWidget *parent, QVBoxLayout *layout);
    void buildGlassCard(QWidget *parent, QVBoxLayout *layout);
    void buildCssCard(QWidget *parent, QVBoxLayout *layout);

    void refreshThemes();
    void refreshControls();
    void applyCustomCssNow();
    void scheduleCustomCssApply();
    void downloadBackground(const QString &url);
    void installThemeFromDisk();
    void exportCurrentTheme();
    void removeSelectedTheme();

    QGridLayout *m_themeGrid = nullptr;
    QLabel *m_themeHint = nullptr;
    QLabel *m_accentLabel = nullptr;
    QCheckBox *m_accentFromCover = nullptr;
    QLineEdit *m_backgroundPath = nullptr;
    QSlider *m_backgroundBlur = nullptr;
    QComboBox *m_fontCombo = nullptr;
    QSlider *m_fontScale = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QSlider *m_radiusSlider = nullptr;
    QSlider *m_glassSlider = nullptr;
    QSlider *m_blurSlider = nullptr;
    QCheckBox *m_animations = nullptr;
    QCheckBox *m_frameless = nullptr;
    QCheckBox *m_cssEnabled = nullptr;
    QPlainTextEdit *m_cssEditor = nullptr;
    QLabel *m_cssProblems = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_cssTimer = nullptr;
    QList<ThemeTile *> m_tiles;
};

#endif // APPEARANCEPAGE_H