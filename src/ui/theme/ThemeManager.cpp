#include "ThemeManager.h"

#include "StyleSheet.h"

#include "core/media/CoverPalette.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {

const char *kThemeKey = "appearance/theme";
const char *kCustomCssKey = "appearance/customCss";
const char *kCustomCssEnabledKey = "appearance/customCssEnabled";
const char *kAccentKey = "appearance/accent";
const char *kAccentFromCoverKey = "appearance/accentFromCover";
const char *kBgImageKey = "appearance/backgroundImage";
const char *kBgBlurKey = "appearance/backgroundBlur";
const char *kRadiusKey = "appearance/radius";
const char *kGlassKey = "appearance/glassAlpha";
const char *kBlurKey = "appearance/blur";
const char *kFontKey = "appearance/font";
const char *kFontScaleKey = "appearance/fontScale";
const char *kModeKey = "appearance/mode";
const char *kAnimationsKey = "appearance/animations";

QString copyDirectory(const QString &from, const QString &to, QString *error)
{
    QDir source(from);
    if (!source.exists()) {
        if (error)
            *error = QObject::tr("Папка не найдена: %1").arg(from);
        return QString();
    }

    QDir target(to);
    if (target.exists()) {
        if (error)
            *error = QObject::tr("Тема уже установлена: %1").arg(to);
        return QString();
    }
    QDir().mkpath(to);

    const QFileInfoList entries = source.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                                       QDir::DirsFirst);
    for (const QFileInfo &entry : entries) {
        const QString targetPath = target.filePath(entry.fileName());
        if (entry.isDir()) {
            if (entry.fileName().compare(QLatin1String("assets"), Qt::CaseInsensitive) == 0) {
                if (copyDirectory(entry.absoluteFilePath(), targetPath, error).isEmpty())
                    return QString();
            }
            continue;
        }
        if (QFile::exists(targetPath))
            QFile::remove(targetPath);
        if (!QFile::copy(entry.absoluteFilePath(), targetPath)) {
            if (error)
                *error = QObject::tr("Не удалось скопировать %1").arg(entry.fileName());
            return QString();
        }
    }
    return to;
}

} // namespace

ThemeManager *ThemeManager::s_instance = nullptr;
bool ThemeManager::s_safeMode = false;

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    loadThemes();
    if (!s_instance)
        s_instance = this;
}

ThemeManager *ThemeManager::instance()
{
    return s_instance;
}

void ThemeManager::setInstance(ThemeManager *manager)
{
    s_instance = manager;
}

bool ThemeManager::safeMode()
{
    return s_safeMode;
}

void ThemeManager::setSafeMode(bool enabled)
{
    s_safeMode = enabled;
}

// --- каталог тем ------------------------------------------------------------

QString ThemeManager::bundledThemesDir()
{
    return QStringLiteral(":/themes");
}

QString ThemeManager::userThemesDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir = QDir(base.isEmpty() ? QDir::homePath() : base)
                            .filePath(QStringLiteral("themes"));
    QDir().mkpath(dir);
    return dir;
}

void ThemeManager::loadThemes()
{
    m_themes.clear();

    // Встроенные темы: ресурсы :/themes/<id>/theme.json
    QDir bundled(bundledThemesDir());
    const QStringList bundledDirs = bundled.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &entry : bundledDirs) {
        QString error;
        const Theme theme = Theme::loadFromDir(bundled.filePath(entry), true, &error);
        if (theme.isValid())
            m_themes.append(theme);
    }

    // Пользовательские темы из папки приложения.
    const QString userDir = userThemesDir();
    QDir user(userDir);
    const QStringList userDirs = user.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &entry : userDirs) {
        QString error;
        const Theme theme = Theme::loadFromDir(user.filePath(entry), false, &error);
        if (!theme.isValid())
            continue;

        // Своя тема с тем же id перекрывает встроенную.
        for (int i = 0; i < m_themes.size(); ++i) {
            if (m_themes.at(i).id == theme.id) {
                m_themes.removeAt(i);
                break;
            }
        }
        m_themes.append(theme);
    }

    if (m_themes.isEmpty()) {
        QString error;
        const Theme fallback = Theme::fromJson(QJsonDocument::fromJson(R"({
            "id": "liquid-glass", "name": "Liquid Glass", "version": "1.0.0",
            "description": { "ru": "Стекло и мягкий свет" },
            "author": { "name": "FLACORE" },
            "accent": "#ff5500", "background": "#0b0d13", "font": "segui"
        })").object(),
                                                QString(), true, &error);
        if (fallback.isValid())
            m_themes.append(fallback);
    }
}

QList<Theme> ThemeManager::themes() const
{
    return m_themes;
}

Theme ThemeManager::theme(const QString &id) const
{
    for (const Theme &item : m_themes) {
        if (item.id == id)
            return item;
    }
    return m_themes.isEmpty() ? Theme() : m_themes.first();
}

QString ThemeManager::currentThemeId() const
{
    const QString stored = settingsValue(kThemeKey);
    if (Theme::isValidId(stored) && (!theme(stored).id.isEmpty()))
        return stored;
    return defaultThemeId();
}

Theme ThemeManager::currentTheme() const
{
    return theme(currentThemeId());
}

void ThemeManager::setCurrentTheme(const QString &id)
{
    if (theme(id).id.isEmpty())
        return;
    setSettingsValue(kThemeKey, id);
    apply();
}

void ThemeManager::reloadThemes()
{
    loadThemes();
    emit themeListChanged();
    apply();
}

void ThemeManager::openThemesFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(userThemesDir()));
}

bool ThemeManager::installTheme(const QString &sourcePath, QString *error)
{
    const QFileInfo info(sourcePath);
    QString themeDir;

    if (info.isDir()) {
        themeDir = info.absoluteFilePath();
    } else {
        // Одиночный theme.json: кладём в папку с id из файла.
        QFile file(sourcePath);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error)
                *error = QObject::tr("Не удалось открыть %1").arg(sourcePath);
            return false;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QString id = doc.object().value(QStringLiteral("id")).toString();
        if (!Theme::isValidId(id, error))
            return false;

        const QString target = QDir(userThemesDir()).filePath(id);
        QDir().mkpath(target);
        const QString jsonTarget = QDir(target).filePath(QStringLiteral("theme.json"));
        if (QFile::exists(jsonTarget))
            QFile::remove(jsonTarget);
        if (!file.copy(jsonTarget)) {
            if (error)
                *error = QObject::tr("Не удалось скопировать theme.json");
            return false;
        }
        loadThemes();
        emit themeListChanged();
        emit themeInstalled(id);
        return true;
    }

    QString parseError;
    const Theme probe = Theme::loadFromDir(themeDir, false, &parseError);
    if (!probe.isValid()) {
        if (error)
            *error = parseError;
        return false;
    }

    const QString target = QDir(userThemesDir()).filePath(probe.id);
    if (QDir(target).exists()) {
        if (error)
            *error = QObject::tr("Тема «%1» уже установлена — удалите её или измените id")
                         .arg(probe.id);
        return false;
    }
    if (copyDirectory(themeDir, target, error).isEmpty())
        return false;

    loadThemes();
    emit themeListChanged();
    emit themeInstalled(probe.id);
    return true;
}

bool ThemeManager::exportCurrentTheme(const QString &targetPath, QString *error)
{
    const Theme theme = currentTheme();
    if (!theme.isValid()) {
        if (error)
            *error = QObject::tr("Нет активной темы");
        return false;
    }

    QDir dir(targetPath);
    if (!dir.exists() && !QDir().mkpath(targetPath)) {
        if (error)
            *error = QObject::tr("Не удалось создать папку %1").arg(targetPath);
        return false;
    }

    // Тема, которой управляет пользователь, выгружается целиком: JSON + CSS.
    QFile json(QDir(targetPath).filePath(QStringLiteral("theme.json")));
    if (!json.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QObject::tr("Не удалось записать theme.json");
        return false;
    }
    const QJsonDocument doc(theme.toJson());
    json.write(doc.toJson(QJsonDocument::Indented));
    json.close();

    if (!theme.css.trimmed().isEmpty()) {
        QFile css(QDir(targetPath).filePath(QStringLiteral("theme.qss")));
        if (css.open(QIODevice::WriteOnly | QIODevice::Truncate))
            css.write(theme.css.toUtf8());
    }
    return true;
}

bool ThemeManager::removeUserTheme(const QString &id, QString *error)
{
    const Theme target = theme(id);
    if (!target.isValid() || target.builtin) {
        if (error)
            *error = QObject::tr("Встроенную тему удалить нельзя");
        return false;
    }

    QDir dir(target.dirPath);
    if (!dir.removeRecursively()) {
        if (error)
            *error = QObject::tr("Не удалось удалить папку темы");
        return false;
    }

    if (currentThemeId() == id)
        setSettingsValue(kThemeKey, defaultThemeId());
    loadThemes();
    emit themeListChanged();
    apply();
    return true;
}

// --- настройки вида ---------------------------------------------------------

QString ThemeManager::settingsValue(const char *key, const QString &fallback) const
{
    return QSettings().value(QString::fromLatin1(key), fallback).toString();
}

void ThemeManager::setSettingsValue(const char *key, const QVariant &value)
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(key), value);
    settings.sync();
}

QString ThemeManager::accentOverride() const
{
    const QString value = settingsValue(kAccentKey).trimmed();
    return QColor(value).isValid() ? QColor(value).name(QColor::HexRgb) : QString();
}

void ThemeManager::setAccentOverride(const QString &color)
{
    const QColor parsed(color);
    setSettingsValue(kAccentKey, parsed.isValid() ? parsed.name(QColor::HexRgb) : QString());
    apply();
}

bool ThemeManager::accentFromCover() const
{
    return QSettings().value(QString::fromLatin1(kAccentFromCoverKey), false).toBool();
}

void ThemeManager::setAccentFromCover(bool enabled)
{
    setSettingsValue(kAccentFromCoverKey, enabled);
    apply();
}

QString ThemeManager::backgroundImage() const
{
    return settingsValue(kBgImageKey);
}

void ThemeManager::setBackgroundImage(const QString &path)
{
    setSettingsValue(kBgImageKey, path);
    apply();
}

int ThemeManager::backgroundBlur() const
{
    return QSettings().value(QString::fromLatin1(kBgBlurKey), -1).toInt();
}

void ThemeManager::setBackgroundBlur(int blur)
{
    setSettingsValue(kBgBlurKey, blur);
    apply();
}

int ThemeManager::radiusOverride() const
{
    return QSettings().value(QString::fromLatin1(kRadiusKey), -1).toInt();
}

void ThemeManager::setRadiusOverride(int radius)
{
    setSettingsValue(kRadiusKey, radius);
    apply();
}

int ThemeManager::glassAlphaPercent() const
{
    return QSettings().value(QString::fromLatin1(kGlassKey), -1).toInt();
}

void ThemeManager::setGlassAlphaPercent(int percent)
{
    setSettingsValue(kGlassKey, percent);
    apply();
}

int ThemeManager::blurOverride() const
{
    return QSettings().value(QString::fromLatin1(kBlurKey), -1).toInt();
}

void ThemeManager::setBlurOverride(int blur)
{
    setSettingsValue(kBlurKey, blur);
    apply();
}

QString ThemeManager::fontOverride() const
{
    return settingsValue(kFontKey);
}

void ThemeManager::setFontOverride(const QString &fontId)
{
    setSettingsValue(kFontKey, fontId);
    apply();
}

int ThemeManager::fontScalePercent() const
{
    return QSettings().value(QString::fromLatin1(kFontScaleKey), 100).toInt();
}

void ThemeManager::setFontScalePercent(int percent)
{
    setSettingsValue(kFontScaleKey, qBound(80, percent, 150));
    apply();
}

QString ThemeManager::mode() const
{
    const QString value = settingsValue(kModeKey);
    if (value.isEmpty())
        return currentTheme().mode;
    return value;
}

void ThemeManager::setMode(const QString &mode)
{
    setSettingsValue(kModeKey, mode);
    apply();
}

int ThemeManager::animationsOverride() const
{
    return QSettings().value(QString::fromLatin1(kAnimationsKey), -1).toInt();
}

void ThemeManager::setAnimationsOverride(int value)
{
    setSettingsValue(kAnimationsKey, value);
    apply();
}

QString ThemeManager::customCss() const
{
    return settingsValue(kCustomCssKey);
}

void ThemeManager::setCustomCss(const QString &css)
{
    setSettingsValue(kCustomCssKey, css);
    apply();
}

bool ThemeManager::customCssEnabled() const
{
    return QSettings().value(QString::fromLatin1(kCustomCssEnabledKey), true).toBool();
}

void ThemeManager::setCustomCssEnabled(bool enabled)
{
    setSettingsValue(kCustomCssEnabledKey, enabled);
    apply();
}

// --- применение -------------------------------------------------------------

AppearanceOverrides ThemeManager::overrides() const
{
    AppearanceOverrides result;
    result.accent = accentOverride();
    result.backgroundImage = backgroundImage();
    result.backgroundBlur = backgroundBlur();
    result.radius = radiusOverride();
    result.glassAlphaPercent = glassAlphaPercent();
    result.blur = blurOverride();
    result.font = fontOverride();
    result.fontScalePercent = fontScalePercent();
    result.mode = settingsValue(kModeKey);
    result.animations = animationsOverride();
    return result;
}

void ThemeManager::applyPalette(QApplication *app) const
{
    QPalette palette = app->palette();
    const QColor background = m_tokens.background;
    const QColor text = m_tokens.textPrimary;
    const QColor accent = m_tokens.accent;

    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, m_tokens.isLight ? background.lighter(103)
                                                      : background.darker(115));
    palette.setColor(QPalette::AlternateBase, background.darker(105));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, background);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, readableOn(accent));
    palette.setColor(QPalette::ToolTipBase, background.lighter(110));
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::PlaceholderText, mixColors(text, background, 0.55));
    palette.setColor(QPalette::Disabled, QPalette::Text, mixColors(text, background, 0.6));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, mixColors(text, background, 0.6));

    app->setPalette(palette);
}

void ThemeManager::apply()
{
    QApplication *app = qApp;
    if (!app)
        return;

    const Theme active = currentTheme();
    AppearanceOverrides effective = overrides();

    if (s_safeMode || m_escapeHatchOff) {
        // Аварийный режим: тема по умолчанию, свой CSS выключен.
        Theme safe = theme(defaultThemeId());
        if (!safe.isValid())
            safe = active;
        safe.css.clear();
        m_tokens = resolveTokens(safe, effective);
        const StyleSheet::BuildResult result = StyleSheet::build(safe, m_tokens, QString(), false);
        app->setStyleSheet(result.qss);
        m_problems = QStringList();
    } else {
        m_tokens = resolveTokens(active, effective);
        const bool withCustom = customCssEnabled();
        const StyleSheet::BuildResult result
            = StyleSheet::build(active, m_tokens, customCss(), withCustom);
        app->setStyleSheet(result.qss);
        m_problems = result.problems;
    }

    // Шрифт: семейство из темы, размер — уже с учётом масштаба в токенах.
    QFont font = app->font();
    const QString family = m_tokens.values.value(QStringLiteral("font-sans"));
    if (!family.isEmpty())
        font.setFamily(family);
    font.setPointSize(baseFontPointSize(m_tokens));
    app->setFont(font);

    applyPalette(app);

    emit changed();
    emit accentChanged(m_tokens.accent);
    emit problemsChanged(m_problems);
}

void ThemeManager::resetAppearance()
{
    QSettings settings;
    const QStringList keys = {
        QString::fromLatin1(kThemeKey),          QString::fromLatin1(kAccentKey),
        QString::fromLatin1(kAccentFromCoverKey), QString::fromLatin1(kBgImageKey),
        QString::fromLatin1(kBgBlurKey),         QString::fromLatin1(kRadiusKey),
        QString::fromLatin1(kGlassKey),          QString::fromLatin1(kBlurKey),
        QString::fromLatin1(kFontKey),           QString::fromLatin1(kFontScaleKey),
        QString::fromLatin1(kModeKey),           QString::fromLatin1(kAnimationsKey),
        QString::fromLatin1(kCustomCssEnabledKey),
    };
    for (const QString &key : keys)
        settings.remove(key);

    // Текст своего CSS сохраняем (его можно включить снова), но выключаем:
    // «сбросить оформление» должно возвращать вид по умолчанию без сюрпризов
    // от пользовательских правил.
    settings.setValue(QString::fromLatin1(kCustomCssEnabledKey), false);
    settings.sync();

    m_escapeHatchOff = false;
    emit themeListChanged();
    apply();
}

void ThemeManager::toggleEscapeHatch()
{
    if (!m_escapeHatchOff) {
        m_previousThemeId = currentThemeId();
        m_previousCustomCss = customCssEnabled();
        m_escapeHatchOff = true;
    } else {
        m_escapeHatchOff = false;
        if (!m_previousThemeId.isEmpty())
            setSettingsValue(kThemeKey, m_previousThemeId);
        setSettingsValue(kCustomCssEnabledKey, m_previousCustomCss);
    }
    apply();
}

void ThemeManager::applyCoverPalette(const QImage &cover)
{
    if (!accentFromCover() || cover.isNull())
        return;

    const CoverPalette::Palette palette = CoverPalette::extract(cover);
    if (!palette.valid)
        return;

    // Ставим акцент обложки как переопределение — тема остаётся, меняется оттенок.
    setSettingsValue(kAccentKey, palette.accent.name(QColor::HexRgb));
    apply();
}