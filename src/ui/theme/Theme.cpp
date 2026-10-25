#include "Theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QObject>
#include <QRegularExpression>
#include <QSettings>
#include <QtMath>

#include <algorithm>

namespace {

QString colorString(const QJsonValue &value)
{
    if (!value.isString())
        return QString();
    const QString text = value.toString().trimmed().toLower();
    if (!QRegularExpression(QStringLiteral("^#[0-9a-f]{6}$")).match(text).hasMatch())
        return QString();
    return text;
}

double luminance(const QColor &color)
{
    return (0.2126 * color.redF() + 0.7152 * color.greenF() + 0.0722 * color.blueF());
}

/// Первое семейство из списка, которое реально установлено в системе.
QString pickInstalledFamily(const QStringList &candidates)
{
    if (candidates.isEmpty())
        return QString();

    const QStringList installed = QFontDatabase::families();
    for (const QString &candidate : candidates) {
        for (const QString &family : installed) {
            if (family.compare(candidate, Qt::CaseInsensitive) == 0)
                return family;
        }
    }
    // Ничего из списка нет — оставляем как есть: Qt подберёт системную замену.
    return candidates.first();
}

QString tokenDefault(const QString &key, bool light, const QColor &accent, const QColor &bg,
                     const QString &fontFamily)
{
    const QColor text = light ? QColor(0x15, 0x17, 0x1d) : QColor(0xf2, 0xf4, 0xf8);
    const QColor textMuted = mixColors(text, bg, light ? 0.30 : 0.38);
        const QColor textFaint = mixColors(text, bg, light ? 0.48 : 0.52);

    if (key == QLatin1String("accent"))
        return toCssColor(accent);
    if (key == QLatin1String("accent-hover"))
        return toCssColor(lightenColor(accent, 0.12));
    if (key == QLatin1String("accent-glow"))
        return toCssColor(withAlpha(accent, 0.38));
    if (key == QLatin1String("accent-soft"))
        return toCssColor(withAlpha(accent, 0.16));
    if (key == QLatin1String("accent-contrast"))
        return toCssColor(readableOn(accent));
    if (key == QLatin1String("bg-primary"))
        return toCssColor(bg);
    if (key == QLatin1String("bg-elevated"))
        return toCssColor(light ? darkenColor(bg, 0.05) : lightenColor(bg, 0.07));
    if (key == QLatin1String("panel"))
        return toCssColor(light ? withAlpha(QColor(255, 255, 255), 0.72)
                                : withAlpha(QColor(255, 255, 255), 0.055));
    if (key == QLatin1String("panel-strong"))
        return toCssColor(light ? withAlpha(QColor(255, 255, 255), 0.92)
                                : withAlpha(QColor(255, 255, 255), 0.09));
    if (key == QLatin1String("panel-solid"))
        return toCssColor(light ? darkenColor(bg, 0.03) : lightenColor(bg, 0.045));
    if (key == QLatin1String("border") || key == QLatin1String("panel-border"))
        return toCssColor(light ? withAlpha(QColor(0x10, 0x12, 0x1a), 0.10)
                                : withAlpha(QColor(255, 255, 255), 0.09));
    if (key == QLatin1String("border-strong"))
        return toCssColor(withAlpha(accent, 0.42));
    if (key == QLatin1String("text"))
        return toCssColor(text);
    if (key == QLatin1String("text-muted"))
        return toCssColor(textMuted);
    if (key == QLatin1String("text-faint"))
        return toCssColor(textFaint);
    if (key == QLatin1String("hover-bg"))
        return toCssColor(light ? withAlpha(QColor(0x10, 0x12, 0x1a), 0.05)
                                : withAlpha(QColor(255, 255, 255), 0.06));
    if (key == QLatin1String("sel-bg"))
        return toCssColor(withAlpha(accent, light ? 0.18 : 0.14));
    if (key == QLatin1String("shadow"))
        return toCssColor(light ? withAlpha(QColor(0x0f, 0x12, 0x1a), 0.16)
                                : withAlpha(QColor(0, 0, 0), 0.46));
    if (key == QLatin1String("danger"))
            return toCssColor(light ? QColor(0xc0, 0x39, 0x2b) : QColor(0xff, 0x6b, 0x6b));
        if (key == QLatin1String("danger-soft"))
            return toCssColor(withAlpha(light ? QColor(0xc0, 0x39, 0x2b) : QColor(0xff, 0x6b, 0x6b),
                                        light ? 0.12 : 0.14));
        if (key == QLatin1String("danger-border"))
            return toCssColor(
                withAlpha(light ? QColor(0xc0, 0x39, 0x2b) : QColor(0xff, 0x6b, 0x6b), 0.45));
        if (key == QLatin1String("success"))
            return toCssColor(light ? QColor(0x1b, 0x7f, 0x4b) : QColor(0x4a, 0xd2, 0x95));
    if (key == QLatin1String("font-sans"))
        return fontFamily;
    if (key == QLatin1String("font-size"))
        return QStringLiteral("10pt");
    if (key == QLatin1String("font-size-sm"))
        return QStringLiteral("9pt");
    if (key == QLatin1String("font-size-lg"))
        return QStringLiteral("12pt");
    if (key == QLatin1String("font-size-title"))
        return QStringLiteral("15pt");
    if (key == QLatin1String("radius"))
        return QStringLiteral("16");
    if (key == QLatin1String("radius-sm"))
        return QStringLiteral("10");
    if (key == QLatin1String("radius-lg"))
        return QStringLiteral("22");
    if (key == QLatin1String("glass-alpha"))
        return QStringLiteral("0.55");
    if (key == QLatin1String("blur"))
        return QStringLiteral("24");
    if (key == QLatin1String("titlebar-height"))
        return QStringLiteral("44");
    if (key == QLatin1String("sidebar-width"))
        return QStringLiteral("216");
    if (key == QLatin1String("player-height"))
        return QStringLiteral("86");
    if (key == QLatin1String("mode"))
        return QStringLiteral("beauty");
    if (key == QLatin1String("animations"))
        return QStringLiteral("1");
    if (key == QLatin1String("bg-image"))
        return QString();
    if (key == QLatin1String("bg-blur"))
        return QStringLiteral("32");
    return QString();
}

bool tokenIsColor(const QString &key)
{
    static const QStringList keys = {
        QStringLiteral("accent"),         QStringLiteral("accent-hover"),
        QStringLiteral("accent-glow"),    QStringLiteral("accent-soft"),
        QStringLiteral("accent-contrast"), QStringLiteral("bg-primary"),
        QStringLiteral("bg-elevated"),    QStringLiteral("panel"),
        QStringLiteral("panel-strong"),   QStringLiteral("panel-solid"),
        QStringLiteral("border"),         QStringLiteral("panel-border"),
        QStringLiteral("border-strong"),  QStringLiteral("text"),
        QStringLiteral("text-muted"),     QStringLiteral("text-faint"),
        QStringLiteral("hover-bg"),       QStringLiteral("sel-bg"),
        QStringLiteral("shadow"),         QStringLiteral("danger"),
        QStringLiteral("success"),
    };
    return keys.contains(key);
}

/// Числовые токены приводим к int, чтобы переопределения из настроек были единообразны.
bool tokenIsNumber(const QString &key)
{
    static const QStringList keys = {
        QStringLiteral("radius"),   QStringLiteral("radius-sm"), QStringLiteral("radius-lg"),
        QStringLiteral("blur"),     QStringLiteral("bg-blur"),   QStringLiteral("titlebar-height"),
        QStringLiteral("sidebar-width"), QStringLiteral("player-height"),
    };
    return keys.contains(key);
}

QString normalizedTokenValue(const QString &key, const QVariant &value)
{
    if (tokenIsColor(key)) {
        const QColor color = value.value<QColor>().isValid() ? value.value<QColor>()
                                                            : QColor(value.toString());
        if (color.isValid())
            return toCssColor(color);
        return value.toString();
    }
    if (tokenIsNumber(key)) {
        bool ok = false;
        const int number = value.toString().toInt(&ok);
        return ok ? QString::number(number) : value.toString();
    }
    if (key == QLatin1String("glass-alpha")) {
        bool ok = false;
        const double number = value.toString().toDouble(&ok);
        return ok ? QString::number(number, 'f', 2) : value.toString();
    }
    return value.toString();
}

} // namespace

// --- цветовая арифметика ----------------------------------------------------

QColor mixColors(const QColor &a, const QColor &b, double t)
{
    t = qBound(0.0, t, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QColor lightenColor(const QColor &color, double amount)
{
    return mixColors(color, QColor(255, 255, 255), amount);
}

QColor darkenColor(const QColor &color, double amount)
{
    return mixColors(color, QColor(0, 0, 0), amount);
}

QColor withAlpha(const QColor &color, double alpha)
{
    QColor out = color;
    out.setAlphaF(qBound(0.0, alpha, 1.0));
    return out;
}

QColor readableOn(const QColor &background)
{
    return luminance(background) > 0.6 ? QColor(0x10, 0x12, 0x16) : QColor(0xff, 0xff, 0xff);
}

QString toCssColor(const QColor &color)
{
    if (color.alpha() == 255)
        return color.name(QColor::HexRgb);
    // Qt читает #AARRGGBB (альфа первая) — надёжнее, чем rgba() в QSS.
    return QStringLiteral("#%1%2%3%4")
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'));
}

// --- шрифты -----------------------------------------------------------------

QStringList availableFontIds()
{
    return {QStringLiteral("segui"),      QStringLiteral("inter"),    QStringLiteral("manrope"),
            QStringLiteral("onest"),      QStringLiteral("golos"),    QStringLiteral("montserrat"),
            QStringLiteral("rubik"),      QStringLiteral("nunito"),   QStringLiteral("raleway"),
            QStringLiteral("comfortaa"),  QStringLiteral("system"),   QStringLiteral("custom")};
}

QStringList fontFamiliesFor(const QString &fontId)
{
    const QString id = fontId.trimmed().toLower();
    if (id == QLatin1String("inter"))
        return {QStringLiteral("Inter"), QStringLiteral("Segoe UI"), QStringLiteral("Noto Sans")};
    if (id == QLatin1String("manrope"))
        return {QStringLiteral("Manrope"), QStringLiteral("Segoe UI"), QStringLiteral("Noto Sans")};
    if (id == QLatin1String("onest"))
        return {QStringLiteral("Onest"), QStringLiteral("Segoe UI"), QStringLiteral("Noto Sans")};
    if (id == QLatin1String("golos"))
        return {QStringLiteral("Golos Text"), QStringLiteral("Segoe UI"), QStringLiteral("Noto Sans")};
    if (id == QLatin1String("montserrat"))
        return {QStringLiteral("Montserrat"), QStringLiteral("Segoe UI")};
    if (id == QLatin1String("rubik"))
        return {QStringLiteral("Rubik"), QStringLiteral("Segoe UI")};
    if (id == QLatin1String("nunito"))
        return {QStringLiteral("Nunito"), QStringLiteral("Segoe UI")};
    if (id == QLatin1String("raleway"))
        return {QStringLiteral("Raleway"), QStringLiteral("Segoe UI")};
    if (id == QLatin1String("comfortaa"))
        return {QStringLiteral("Comfortaa"), QStringLiteral("Segoe UI")};
    if (id == QLatin1String("system"))
        return {};
    // "custom" — пользователь вводит имя семейства вручную (хранится в QSettings).
    // Семейство берётся из настройки appearance/customFontFamily; возвращаем пустой
    // список, чтобы вызывающий код использовал значение напрямую через QSettings.
    if (id == QLatin1String("custom"))
        return {};
    // По умолчанию — системный шрифт, самый предсказуемый на Windows.
    return {QStringLiteral("Segoe UI Variable Text"), QStringLiteral("Segoe UI"),
            QStringLiteral("Noto Sans")};
}

// --- Theme ------------------------------------------------------------------

QString Theme::description() const
{
    if (!descriptionRu.isEmpty())
        return descriptionRu;
    return descriptionEn;
}

QString Theme::previewPath() const
{
    if (dirPath.isEmpty())
        return QString();

    const QStringList candidates = {
        QStringLiteral("preview.webp"), QStringLiteral("preview.png"),
        QStringLiteral("preview.jpg"),  QStringLiteral("preview.jpeg"),
    };
    for (const QString &name : candidates) {
        const QString path = QDir(dirPath).filePath(name);
        if (QFileInfo::exists(path))
            return path;
    }
    return QString();
}

bool Theme::isValidId(const QString &id, QString *error)
{
    const QRegularExpression re(QStringLiteral("^[a-z0-9][a-z0-9-]{1,30}[a-z0-9]$"));
    if (!re.match(id).hasMatch()) {
        if (error)
            *error = QObject::tr("id: 3–32 символа, только a-z, 0-9 и «-», без пробелов");
        return false;
    }
    if (id == QLatin1String("tools")) {
        if (error)
            *error = QObject::tr("id «tools» зарезервирован");
        return false;
    }
    return true;
}

Theme Theme::fromJson(const QJsonObject &object, const QString &dirPath, bool builtin,
                      QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return Theme();
    };

    Theme theme;
    theme.dirPath = dirPath;
    theme.builtin = builtin;

    theme.id = object.value(QStringLiteral("id")).toString().trimmed();
    theme.name = object.value(QStringLiteral("name")).toString().trimmed();
    theme.version = object.value(QStringLiteral("version")).toString().trimmed();
    theme.license = object.value(QStringLiteral("license")).toString().trimmed();
    theme.mode = object.value(QStringLiteral("mode")).toString().trimmed().toLower();

    const QJsonObject description = object.value(QStringLiteral("description")).toObject();
    theme.descriptionRu = description.value(QStringLiteral("ru")).toString().trimmed();
    theme.descriptionEn = description.value(QStringLiteral("en")).toString().trimmed();

    const QJsonObject author = object.value(QStringLiteral("author")).toObject();
    theme.authorName = author.value(QStringLiteral("name")).toString().trimmed();
    theme.authorGithub = author.value(QStringLiteral("github")).toString().trimmed();

    if (theme.id.isEmpty())
        return fail(QObject::tr("нет поля «id»"));
    if (!isValidId(theme.id, error))
        return Theme();
    if (theme.name.isEmpty())
        return fail(QObject::tr("нет поля «name»"));
    if (theme.name.size() > 32)
        return fail(QObject::tr("«name» длиннее 32 символов"));
    if (theme.descriptionRu.isEmpty() && theme.descriptionEn.isEmpty())
        return fail(QObject::tr("нужно описание: «description» с ru или en"));
    if (theme.authorName.isEmpty())
        return fail(QObject::tr("нет автора: «author.name»"));

    const QString accent = colorString(object.value(QStringLiteral("accent")));
    const QString background = colorString(object.value(QStringLiteral("background")));
    theme.accent = accent.isEmpty() ? QStringLiteral("#ff5500") : accent;
    theme.background = background.isEmpty() ? QStringLiteral("#0b0d13") : background;
    theme.font = object.value(QStringLiteral("font")).toString().trimmed().toLower();
    if (theme.font.isEmpty() || !availableFontIds().contains(theme.font))
        theme.font = QStringLiteral("segui");
    if (theme.mode.isEmpty())
        theme.mode = QStringLiteral("beauty");
    if (theme.mode != QLatin1String("beauty") && theme.mode != QLatin1String("medium")
        && theme.mode != QLatin1String("light"))
        theme.mode = QStringLiteral("beauty");
    if (theme.version.isEmpty())
        theme.version = QStringLiteral("1.0.0");

    const QJsonObject tokens = object.value(QStringLiteral("tokens")).toObject();
    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it) {
        const QJsonValue value = it.value();
        if (value.isDouble())
            theme.tokens.insert(it.key(), value.toVariant());
        else if (value.isBool())
            theme.tokens.insert(it.key(), value.toVariant());
        else if (value.isString())
            theme.tokens.insert(it.key(), value.toString());
    }

    // theme.qss лежит рядом с theme.json.
    if (!dirPath.isEmpty()) {
        const QString cssPath = QDir(dirPath).filePath(QStringLiteral("theme.qss"));
        QFile cssFile(cssPath);
        if (cssFile.open(QIODevice::ReadOnly))
            theme.css = QString::fromUtf8(cssFile.readAll());
    }

    return theme;
}

Theme Theme::loadFromDir(const QString &dirPath, bool builtin, QString *error)
{
    const QString jsonPath = QDir(dirPath).filePath(QStringLiteral("theme.json"));
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("нет файла theme.json: %1").arg(jsonPath);
        return Theme();
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (error)
            *error = QObject::tr("ошибка JSON: %1").arg(parseError.errorString());
        return Theme();
    }
    return fromJson(doc.object(), dirPath, builtin, error);
}

QJsonObject Theme::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("version"), version);
    object.insert(QStringLiteral("license"), license.isEmpty() ? QStringLiteral("MIT") : license);

    QJsonObject description;
    if (!descriptionEn.isEmpty())
        description.insert(QStringLiteral("en"), descriptionEn);
    if (!descriptionRu.isEmpty())
        description.insert(QStringLiteral("ru"), descriptionRu);
    object.insert(QStringLiteral("description"), description);

    QJsonObject author;
    author.insert(QStringLiteral("name"), authorName.isEmpty() ? QStringLiteral("anonymous")
                                                              : authorName);
    if (!authorGithub.isEmpty())
        author.insert(QStringLiteral("github"), authorGithub);
    object.insert(QStringLiteral("author"), author);

    object.insert(QStringLiteral("accent"), accent);
    object.insert(QStringLiteral("background"), background);
    object.insert(QStringLiteral("font"), font);
    object.insert(QStringLiteral("mode"), mode);

    if (!tokens.isEmpty()) {
        QJsonObject tokenObject;
        for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it) {
            const QVariant value = it.value();
            switch (value.typeId()) {
            case QMetaType::Int:
            case QMetaType::LongLong:
                tokenObject.insert(it.key(), value.toInt());
                break;
            case QMetaType::Double:
                tokenObject.insert(it.key(), value.toDouble());
                break;
            case QMetaType::Bool:
                tokenObject.insert(it.key(), value.toBool());
                break;
            default:
                tokenObject.insert(it.key(), value.toString());
                break;
            }
        }
        object.insert(QStringLiteral("tokens"), tokenObject);
    }
    return object;
}

// --- токены -----------------------------------------------------------------

ResolvedTokens resolveTokens(const Theme &theme, const AppearanceOverrides &overrides)
{
    ResolvedTokens resolved;

    const QColor accent = QColor(overrides.accent.isEmpty() ? theme.accent : overrides.accent);
    const QColor background = QColor(theme.background);
    const bool light = luminance(background) > 0.55;

    // Шрифт: id -> первое доступное семейство (или системный).
    QString fontId = overrides.font.isEmpty() ? theme.font : overrides.font;
    const QStringList families = fontFamiliesFor(fontId);
    QString family = pickInstalledFamily(families);

    // Issue #10: поддержка пользовательского шрифта ("custom").
    // Если выбран "custom" — берём имя семейства напрямую из настройки
    // appearance/customFontFamily и проверяем его наличие в системе.
    if (fontId == QLatin1String("custom")) {
        const QString customFamily = QSettings().value(
            QStringLiteral("appearance/customFontFamily")).toString().trimmed();
        if (!customFamily.isEmpty()) {
            // Проверяем, есть ли такое семейство в системе.
            const QStringList installed = QFontDatabase::families();
            const bool found = std::any_of(installed.constBegin(), installed.constEnd(),
                                           [&customFamily](const QString &f) {
                                               return f.compare(customFamily, Qt::CaseInsensitive) == 0;
                                           });
            if (found)
                family = customFamily;
            else
                family = customFamily; // Передаём как есть — Qt сам подберёт замену.
        }
    }

    resolved.isLight = light;
    resolved.accent = accent.isValid() ? accent : QColor(QStringLiteral("#ff5500"));
    resolved.background = background;
    resolved.textPrimary = light ? QColor(0x15, 0x17, 0x1d) : QColor(0xf2, 0xf4, 0xf8);

    const QStringList keys = {
        QStringLiteral("accent"),        QStringLiteral("accent-hover"),
        QStringLiteral("accent-glow"),   QStringLiteral("accent-soft"),
        QStringLiteral("accent-contrast"), QStringLiteral("bg-primary"),
                QStringLiteral("bg-elevated"),   QStringLiteral("panel"),
                QStringLiteral("panel-strong"),  QStringLiteral("panel-solid"),
        QStringLiteral("border"),        QStringLiteral("panel-border"),
        QStringLiteral("border-strong"), QStringLiteral("text"),
        QStringLiteral("text-muted"),    QStringLiteral("text-faint"),
        QStringLiteral("hover-bg"),      QStringLiteral("sel-bg"),
        QStringLiteral("shadow"),        QStringLiteral("danger"),
                QStringLiteral("danger-soft"),   QStringLiteral("danger-border"),
                QStringLiteral("success"),       QStringLiteral("font-sans"),
        QStringLiteral("font-size"),
        QStringLiteral("font-size-sm"),  QStringLiteral("font-size-lg"),
        QStringLiteral("font-size-title"), QStringLiteral("radius"),
        QStringLiteral("radius-sm"),     QStringLiteral("radius-lg"),
        QStringLiteral("glass-alpha"),   QStringLiteral("blur"),
        QStringLiteral("titlebar-height"), QStringLiteral("sidebar-width"),
        QStringLiteral("player-height"), QStringLiteral("mode"),
        QStringLiteral("animations"),    QStringLiteral("bg-image"),
        QStringLiteral("bg-blur"),
    };

    for (const QString &key : keys)
        resolved.values.insert(key, tokenDefault(key, light, accent, background, family));

    // Переопределения темы.
    for (auto it = theme.tokens.constBegin(); it != theme.tokens.constEnd(); ++it)
        resolved.values.insert(it.key(), normalizedTokenValue(it.key(), it.value()));

    // Переопределения пользователя из настроек вида.
    if (!overrides.accent.isEmpty())
        resolved.values.insert(QStringLiteral("accent"), toCssColor(QColor(overrides.accent)));
    if (!overrides.font.isEmpty())
        resolved.values.insert(QStringLiteral("font-sans"), family);
    if (overrides.radius > 0) {
        resolved.values.insert(QStringLiteral("radius"), QString::number(overrides.radius));
        resolved.values.insert(QStringLiteral("radius-sm"),
                               QString::number(qMax(4, overrides.radius - 6)));
        resolved.values.insert(QStringLiteral("radius-lg"),
                               QString::number(overrides.radius + 6));
    }
    if (overrides.blur > 0)
        resolved.values.insert(QStringLiteral("blur"), QString::number(overrides.blur));
    if (overrides.glassAlphaPercent >= 0)
        resolved.values.insert(QStringLiteral("glass-alpha"),
                               QString::number(qBound(0, overrides.glassAlphaPercent, 100) / 100.0,
                                               'f', 2));
    if (!overrides.mode.isEmpty())
        resolved.values.insert(QStringLiteral("mode"), overrides.mode);
    if (!overrides.backgroundImage.isEmpty())
        resolved.values.insert(QStringLiteral("bg-image"), overrides.backgroundImage);
    if (overrides.backgroundBlur > 0)
        resolved.values.insert(QStringLiteral("bg-blur"), QString::number(overrides.backgroundBlur));

    // Акцент мог измениться — пересчитываем производные цвета, если их не задали явно.
    const QColor finalAccent(resolved.values.value(QStringLiteral("accent")));
    if (finalAccent.isValid() && finalAccent != accent) {
        const bool accentOverriddenByTheme = theme.tokens.contains(QStringLiteral("accent"));
        if (!accentOverriddenByTheme) {
            resolved.values.insert(QStringLiteral("accent"), toCssColor(finalAccent));
            resolved.values.insert(QStringLiteral("accent-hover"),
                                   toCssColor(lightenColor(finalAccent, 0.12)));
            resolved.values.insert(QStringLiteral("accent-glow"),
                                   toCssColor(withAlpha(finalAccent, 0.38)));
            resolved.values.insert(QStringLiteral("accent-soft"),
                                   toCssColor(withAlpha(finalAccent, 0.16)));
            resolved.values.insert(QStringLiteral("accent-contrast"),
                                   toCssColor(readableOn(finalAccent)));
        }
        resolved.accent = finalAccent;
    }

    // Акцент, пришедший из обложки (в настройках «брать акцент из обложки»),
    // должен перекрасить и производные токены.
    if (!overrides.accent.isEmpty()) {
        resolved.values.insert(QStringLiteral("accent-hover"),
                               toCssColor(lightenColor(resolved.accent, 0.12)));
        resolved.values.insert(QStringLiteral("accent-glow"),
                               toCssColor(withAlpha(resolved.accent, 0.38)));
        resolved.values.insert(QStringLiteral("accent-soft"),
                               toCssColor(withAlpha(resolved.accent, 0.16)));
        resolved.values.insert(QStringLiteral("accent-contrast"),
                               toCssColor(readableOn(resolved.accent)));
    }

    // Режим «лайт» выключает анимации и часть размытия (как data-perf="light").
    const QString mode = resolved.values.value(QStringLiteral("mode"));
    if (mode == QLatin1String("light") || overrides.animations == 0)
        resolved.values.insert(QStringLiteral("animations"), QStringLiteral("0"));
    else
        resolved.values.insert(QStringLiteral("animations"), QStringLiteral("1"));
    if (mode == QLatin1String("light"))
        resolved.values.insert(QStringLiteral("blur"), QStringLiteral("0"));
    else if (mode == QLatin1String("medium")
             && resolved.values.value(QStringLiteral("blur")).toInt() > 12)
        resolved.values.insert(QStringLiteral("blur"), QStringLiteral("12"));

    // Масштаб шрифта из настроек вида применяется к размерам шрифта.
    const int scale = qBound(80, overrides.fontScalePercent, 150);
    if (scale != 100) {
        const QStringList fontKeys = {QStringLiteral("font-size"), QStringLiteral("font-size-sm"),
                                     QStringLiteral("font-size-lg"),
                                     QStringLiteral("font-size-title")};
        for (const QString &key : fontKeys) {
            QString value = resolved.values.value(key);
            value.remove(QStringLiteral("pt"));
            bool ok = false;
            const double points = value.toDouble(&ok);
            if (ok)
                resolved.values.insert(key,
                                       QString::number(points * scale / 100.0, 'f', 1)
                                           + QStringLiteral("pt"));
        }
    }

    return resolved;
}

QString substituteTokens(const QString &text, const QHash<QString, QString> &tokens)
{
    if (text.isEmpty())
        return text;

    QString out;
    out.reserve(text.size() + 256);

    static const QRegularExpression re(QStringLiteral("@([a-z][a-z0-9-]*)"));
    int last = 0;
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        out += text.mid(last, match.capturedStart() - last);
        const QString name = match.captured(1);
        const auto token = tokens.constFind(name);
        out += (token == tokens.constEnd()) ? match.captured(0) : token.value();
        last = match.capturedEnd();
    }
    out += text.mid(last);
    return out;
}

QString sanitizeUserCss(const QString &css, const QString &themeDir, QStringList *problems)
{
    const auto report = [problems](const QString &message) {
        if (problems)
            problems->append(message);
    };

    QString out = css;
    const int limit = 256 * 1024;
    if (out.size() > limit) {
        out.truncate(limit);
        report(QObject::tr("Стиль обрезан до 256 КБ"));
    }

    const QRegularExpression importRe(QStringLiteral("@import[^;]*;"),
                                      QRegularExpression::CaseInsensitiveOption);
    if (out.contains(importRe)) {
        report(QObject::tr("@import запрещён — стиль не может тянуть внешние файлы"));
        out.remove(importRe);
    }

    const QRegularExpression remoteRe(
        QStringLiteral("url\\(\\s*['\"]?(?:https?:)?//[^)]*\\)"),
        QRegularExpression::CaseInsensitiveOption);
    const int remoteCount = out.count(remoteRe);
    if (remoteCount > 0) {
        out.remove(remoteRe);
        report(QObject::tr("ссылки на сайты в url() вырезаны — только файлы темы"));
    }

    const QRegularExpression behaviorRe(QStringLiteral("behavior\\s*:[^;}]*;?"),
                                        QRegularExpression::CaseInsensitiveOption);
    if (out.contains(behaviorRe)) {
        report(QObject::tr("свойство behavior запрещено"));
        out.remove(behaviorRe);
    }

    // url("assets/...") -> абсолютный путь внутри папки темы.
    if (!themeDir.isEmpty()) {
        const QRegularExpression assetRe(QStringLiteral("url\\(\\s*['\"]?assets/"),
                                         QRegularExpression::CaseInsensitiveOption);
        out.replace(assetRe, QStringLiteral("url(\"%1/assets/").arg(themeDir));
    }
    return out;
}

int baseFontPointSize(const ResolvedTokens &tokens)
{
    QString size = tokens.values.value(QStringLiteral("font-size"), QStringLiteral("10pt"));
    size.remove(QStringLiteral("pt"));
    bool ok = false;
    const double points = size.toDouble(&ok);
    if (!ok)
        return 10;
    return qMax(6, int(qRound(points)));
}