#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * Тема = папка с theme.json (+ theme.qss и assets/) — формат один в один как в
 * SoundCloud Desktop, чтобы темы было легко переносить и писать руками.
 *
 *   themes/<id>/
 *      theme.json   — метаданные и токены
 *      theme.qss    — необязательный CSS поверх базовых стилей
 *      preview.webp — необязательная картинка для карточки темы
 *      assets/      — картинки/шрифты для theme.qss
 *
 * В theme.qss (и в «своём CSS» из настроек) вместо CSS-переменных пишутся
 * токены с @: @accent, @bg-primary, @radius, @font-sans и т. д. Их подставляет
 * приложение (в Qt QSS нет var()).
 */
struct Theme {
    QString id;
    QString name;
    QString version;
    QString license;
    QString mode;           // beauty | medium | light
    QString dirPath;        // ":/themes/<id>" для встроенных, путь на диске для своих
    bool builtin = false;

    QString accent;         // #rrggbb
    QString background;     // #rrggbb
    QString font;           // id шрифта: segui | inter | onest | ...
    QString descriptionRu;
    QString descriptionEn;

    QString authorName;
    QString authorGithub;

    QHash<QString, QVariant> tokens;  // переопределения любых токенов
    QString css;                      // содержимое theme.qss (может быть пустым)

    bool isValid() const { return !id.isEmpty() && !name.isEmpty(); }
    QString description() const;   // ru, если есть, иначе en
    QJsonObject toJson() const;
    QString previewPath() const;   // первый найденный preview.* или пусто

    static Theme fromJson(const QJsonObject &object, const QString &dirPath, bool builtin,
                          QString *error);
    static Theme loadFromDir(const QString &dirPath, bool builtin, QString *error);
    /// Проверка id по правилу каталога: 3–32 символа, a-z 0-9 «-».
    static bool isValidId(const QString &id, QString *error = nullptr);
};

/// Итоговые значения всех токенов: база + переопределения темы + настройки вида.
struct ResolvedTokens {
    QHash<QString, QString> values;
    bool isLight = false;
    QColor accent;
    QColor background;
    QColor textPrimary;
};

/// Настройки вида, которые пользователь меняет поверх темы (хранятся в QSettings).
struct AppearanceOverrides {
    QString accent;              // пусто = акцент темы
    QString backgroundImage;     // путь к файлу или пусто
    int backgroundBlur = -1;     // -1 = значение темы
    int radius = -1;
    int glassAlphaPercent = -1;
    int blur = -1;
    QString font;                // id шрифта, пусто = из темы
    int fontScalePercent = 100;
    QString mode;                // beauty | medium | light, пусто = из темы
    int animations = -1;         // -1 = авто (по mode), 0/1 — принудительно
};

ResolvedTokens resolveTokens(const Theme &theme, const AppearanceOverrides &overrides);

/// Подставляет @token в текст стиля. Неизвестные токены остаются как есть.
QString substituteTokens(const QString &text, const QHash<QString, QString> &tokens);

/// Аварийный разбор «своего CSS»: вырезает @import, ссылки в сеть, behavior;
/// url("assets/...") превращает в абсолютный путь внутри папки темы.
QString sanitizeUserCss(const QString &css, const QString &themeDir, QStringList *problems);

/// Размер шрифта интерфейса в pt (учитывает fontScalePercent).
int baseFontPointSize(const ResolvedTokens &tokens);

/// Семейства шрифтов по id темы (как в каталоге эталонного клиента).
QStringList fontFamiliesFor(const QString &fontId);
QStringList availableFontIds();

// --- цветовая арифметика (используется для вывода недостающих токенов) -------
QColor mixColors(const QColor &a, const QColor &b, double t);
QColor lightenColor(const QColor &color, double amount);
QColor darkenColor(const QColor &color, double amount);
QColor withAlpha(const QColor &color, double alpha);
QColor readableOn(const QColor &background);
/// #rrggbb или #aarrggbb (Qt читает именно ARGB — так надёжнее, чем rgba()).
QString toCssColor(const QColor &color);

#endif // THEME_H