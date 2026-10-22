#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include "Theme.h"

#include <QImage>
#include <QList>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QStringList>

class QApplication;

/**
 * Хранитель внешнего вида: список тем (встроенные из ресурсов + свои из папки
 * пользователя), активная тема, настройки вида поверх темы, «свой CSS».
 *
 * Всё состояние живёт в QSettings, применение — одна точка входа apply():
 * QSS (базовый + theme.qss + свой CSS), QPalette и шрифт приложения.
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);

    static ThemeManager *instance();
    static void setInstance(ThemeManager *manager);

    /// Аварийный режим (--no-custom-css): свой CSS и тема каталога не применяются.
    static bool safeMode();
    static void setSafeMode(bool enabled);

    // --- темы ---------------------------------------------------------------
    QList<Theme> themes() const;                 // встроенные + пользовательские
    Theme theme(const QString &id) const;
    Theme currentTheme() const;
    QString currentThemeId() const;
    QString defaultThemeId() const { return QStringLiteral("liquid-glass"); }

    void setCurrentTheme(const QString &id);
    void reloadThemes();
    void openThemesFolder() const;

    bool installTheme(const QString &sourcePath, QString *error = nullptr);
    bool exportCurrentTheme(const QString &targetPath, QString *error = nullptr);
    bool removeUserTheme(const QString &id, QString *error = nullptr);

    static QString userThemesDir();
    static QString bundledThemesDir();

    // --- настройки вида -----------------------------------------------------
    QString accentOverride() const;
    void setAccentOverride(const QString &color);       // пусто = акцент темы
    bool accentFromCover() const;
    void setAccentFromCover(bool enabled);

    QString backgroundImage() const;
    void setBackgroundImage(const QString &path);
    int backgroundBlur() const;
    void setBackgroundBlur(int blur);

    int radiusOverride() const;                          // -1 = из темы
    void setRadiusOverride(int radius);
    int glassAlphaPercent() const;                       // -1 = из темы
    void setGlassAlphaPercent(int percent);
    int blurOverride() const;                            // -1 = из темы
    void setBlurOverride(int blur);

    QString fontOverride() const;
    void setFontOverride(const QString &fontId);
    int fontScalePercent() const;
    void setFontScalePercent(int percent);

    QString mode() const;
    void setMode(const QString &mode);                   // beauty | medium | light
    int animationsOverride() const;                      // -1 авто
    void setAnimationsOverride(int value);

    // --- свой CSS -----------------------------------------------------------
    QString customCss() const;
    void setCustomCss(const QString &css);
    bool customCssEnabled() const;
    void setCustomCssEnabled(bool enabled);
    QStringList lastProblems() const { return m_problems; }

    // --- применение ---------------------------------------------------------
    AppearanceOverrides overrides() const;
    const ResolvedTokens &tokens() const { return m_tokens; }

    void apply();
    /// Полный сброс вида: тема по умолчанию, свой CSS выключен, акцент из темы.
    void resetAppearance();
    /// Аналог Ctrl+Alt+Shift+C: выключить/вернуть свой CSS и тему каталога.
    void toggleEscapeHatch();

    /// Перекрасить интерфейс под обложку (если включено в настройках).
    void applyCoverPalette(const QImage &cover);

signals:
    void changed();                       // нужно перерисоваться/перечитать токены
    void accentChanged(const QColor &accent);
    void problemsChanged(const QStringList &problems);
    void themeListChanged();
    void themeInstalled(const QString &id);

private:
    void loadThemes();
    QString settingsValue(const char *key, const QString &fallback = QString()) const;
    void setSettingsValue(const char *key, const QVariant &value);
    void applyPalette(QApplication *app) const;

    QList<Theme> m_themes;
    ResolvedTokens m_tokens;
    QStringList m_problems;
    bool m_escapeHatchOff = false;
    QString m_previousThemeId;
    bool m_previousCustomCss = false;

    static ThemeManager *s_instance;
    static bool s_safeMode;
};

#endif // THEMEMANAGER_H