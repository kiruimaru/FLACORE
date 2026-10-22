// Тесты системы тем: разбор theme.json, вывод токенов, подстановка @tokens,
// аварийный разбор «своего CSS» и сборка итогового QSS.
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ui/theme/StyleSheet.h"
#include "ui/theme/Theme.h"
#include "ui/theme/ThemeManager.h"

namespace {

int g_failures = 0;

void check(bool condition, const QString &what)
{
    std::printf("  %s %s\n", condition ? "ok  " : "FAIL", what.toUtf8().constData());
    if (!condition)
        ++g_failures;
}

QJsonObject themeJson(const QString &id, const QString &name, const QString &background,
                      const QString &accent)
{
    QJsonObject description;
    description.insert(QStringLiteral("ru"), QStringLiteral("Описание темы"));

    QJsonObject author;
    author.insert(QStringLiteral("name"), QStringLiteral("Тестер"));

    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("description"), description);
    object.insert(QStringLiteral("version"), QStringLiteral("1.0.0"));
    object.insert(QStringLiteral("author"), author);
    object.insert(QStringLiteral("license"), QStringLiteral("MIT"));
    object.insert(QStringLiteral("accent"), accent);
    object.insert(QStringLiteral("background"), background);
    object.insert(QStringLiteral("font"), QStringLiteral("inter"));
    return object;
}

bool writeText(const QString &path, const QByteArray &data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(data) == data.size();
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("theme-test"));

    std::printf("Темы: разбор и вывод\n");

    // --- проверка id --------------------------------------------------------
    {
        QString error;
        check(Theme::isValidId(QStringLiteral("midnight"), &error), QStringLiteral("id «midnight» принят"));
        check(!Theme::isValidId(QStringLiteral("Midnight"), &error),
              QStringLiteral("id с заглавными отклонён: %1").arg(error));
        check(!Theme::isValidId(QStringLiteral("a"), &error), QStringLiteral("слишком короткий id отклонён"));
        check(!Theme::isValidId(QStringLiteral("tools"), &error), QStringLiteral("id «tools» отклонён"));
    }

    // --- разбор theme.json --------------------------------------------------
    {
        QString error;
        const Theme theme = Theme::fromJson(themeJson(QStringLiteral("midnight"), QStringLiteral("Midnight"),
                                                      QStringLiteral("#070a12"), QStringLiteral("#7aa2ff")),
                                            QString(), false, &error);
        check(theme.isValid(), QStringLiteral("корректный theme.json разобран"));
        check(theme.accent == QLatin1String("#7aa2ff"), QStringLiteral("акцент прочитан"));
        check(theme.background == QLatin1String("#070a12"), QStringLiteral("фон прочитан"));
        check(theme.description() == QStringLiteral("Описание темы"),
              QStringLiteral("описание прочитано"));

        QJsonObject noDescription = themeJson(QStringLiteral("bare"), QStringLiteral("Bare"),
                                             QStringLiteral("#000000"), QStringLiteral("#ffffff"));
        noDescription.remove(QStringLiteral("description"));
        check(!Theme::fromJson(noDescription, QString(), false, &error).isValid(),
              QStringLiteral("theme.json без описания отклонён"));

        QJsonObject longName = themeJson(QStringLiteral("longname"), QStringLiteral("Имя"), QStringLiteral("#101010"),
                                         QStringLiteral("#ff0000"));
        longName.insert(QStringLiteral("name"), QString(40, QLatin1Char('x')));
        check(!Theme::fromJson(longName, QString(), false, &error).isValid(),
              QStringLiteral("слишком длинное имя отклонено"));

        QJsonObject badColor = themeJson(QStringLiteral("badcolor"), QStringLiteral("Bad"),
                                         QStringLiteral("не цвет"), QStringLiteral("#ff0000"));
        const Theme fallback = Theme::fromJson(badColor, QString(), false, &error);
        check(fallback.background == QLatin1String("#0b0d13"),
              QStringLiteral("битый цвет фона заменён значением по умолчанию"));
    }

    // --- пакет темы на диске ------------------------------------------------
    {
        QTemporaryDir dir;
        const QString themeDir = QDir(dir.path()).filePath(QStringLiteral("midnight"));
        QDir().mkpath(themeDir);

        const QJsonDocument doc(themeJson(QStringLiteral("midnight"), QStringLiteral("Midnight"),
                                          QStringLiteral("#070a12"), QStringLiteral("#7aa2ff")));
        check(writeText(QDir(themeDir).filePath(QStringLiteral("theme.json")),
                        doc.toJson(QJsonDocument::Indented)),
              QStringLiteral("theme.json записан в пакет"));
        check(writeText(QDir(themeDir).filePath(QStringLiteral("theme.qss")),
                        QByteArray("QWidget[dataUi=\"player\"] { border-color: @accent; }\n")),
              QStringLiteral("theme.qss записан в пакет"));

        QString error;
        const Theme loaded = Theme::loadFromDir(themeDir, false, &error);
        check(loaded.isValid(), QStringLiteral("пакет темы прочитан: %1").arg(error));
        check(loaded.css.contains(QLatin1String("@accent")), QStringLiteral("theme.qss попал в тему"));
        check(!loaded.builtin, QStringLiteral("пользовательская тема помечена как не встроенная"));

        const QJsonObject roundTrip = loaded.toJson();
        check(roundTrip.value(QStringLiteral("accent")).toString() == QLatin1String("#7aa2ff"),
              QStringLiteral("экспорт темы сохраняет акцент"));
        check(roundTrip.value(QStringLiteral("id")).toString() == QLatin1String("midnight"),
              QStringLiteral("экспорт темы сохраняет id"));
    }

    // --- вывод токенов ------------------------------------------------------
    {
        AppearanceOverrides overrides;
        const Theme dark = Theme::fromJson(themeJson(QStringLiteral("dark"), QStringLiteral("Dark"),
                                                     QStringLiteral("#0b0d13"), QStringLiteral("#ff5500")),
                                           QString(), false, nullptr);
        const ResolvedTokens tokens = resolveTokens(dark, overrides);
        check(!tokens.isLight, QStringLiteral("тёмный фон распознан"));
        check(tokens.values.value(QStringLiteral("accent")) == QLatin1String("#ff5500"),
              QStringLiteral("акцент попал в токены"));
        check(tokens.values.value(QStringLiteral("accent-hover")) != QStringLiteral("#ff5500"),
              QStringLiteral("accent-hover выведен из акцента"));
        check(tokens.values.value(QStringLiteral("text")) == QLatin1String("#f2f4f8"),
              QStringLiteral("текст для тёмной темы светлый"));
        check(tokens.values.value(QStringLiteral("radius")) == QLatin1String("16"),
              QStringLiteral("скругление по умолчанию 16"));

        const Theme light = Theme::fromJson(themeJson(QStringLiteral("light"), QStringLiteral("Light"),
                                                      QStringLiteral("#f4f5fa"), QStringLiteral("#ec4899")),
                                            QString(), false, nullptr);
        const ResolvedTokens lightTokens = resolveTokens(light, overrides);
        check(lightTokens.isLight, QStringLiteral("светлый фон распознан"));
        check(lightTokens.textPrimary.lightness() < 80, QStringLiteral("текст для светлой темы тёмный"));

        AppearanceOverrides custom;
        custom.accent = QStringLiteral("#22d3ee");
        custom.radius = 24;
        custom.glassAlphaPercent = 80;
        custom.mode = QStringLiteral("light");
        const ResolvedTokens overridden = resolveTokens(light, custom);
        check(overridden.values.value(QStringLiteral("accent")) == QLatin1String("#22d3ee"),
              QStringLiteral("акцент из настроек перекрывает тему"));
        check(overridden.values.value(QStringLiteral("radius")) == QLatin1String("24"),
              QStringLiteral("скругление из настроек применяется"));
        check(overridden.values.value(QStringLiteral("glass-alpha")) == QLatin1String("0.80"),
              QStringLiteral("плотность стекла из настроек"));
        check(overridden.values.value(QStringLiteral("blur")) == QLatin1String("0"),
              QStringLiteral("режим «лайт» выключает размытие"));
        check(overridden.values.value(QStringLiteral("animations")) == QLatin1String("0"),
              QStringLiteral("режим «лайт» выключает анимации"));
    }

    // --- подстановка @tokens ------------------------------------------------
    {
        const QHash<QString, QString> tokens = {{QStringLiteral("accent"), QStringLiteral("#ff5500")},
                                                {QStringLiteral("radius"), QStringLiteral("16")}};
        check(substituteTokens(QStringLiteral("color: @accent; border-radius: @radius;"), tokens)
                  == QStringLiteral("color: #ff5500; border-radius: 16;"),
              QStringLiteral("@tokens подставлены"));
        check(substituteTokens(QStringLiteral("color: @unknown;"), tokens)
                  == QStringLiteral("color: @unknown;"),
              QStringLiteral("неизвестный токен остаётся как есть"));
    }

    // --- аварийный разбор своего CSS ---------------------------------------
    {
        QStringList problems;
        const QString cleaned = sanitizeUserCss(
            QStringLiteral("@import url('http://evil/x.css');\n"
                           "QWidget { border-image: url(https://evil/bg.png); }\n"
                           "body { behavior: url(#default#x); }\n"
                           "QLabel { background-image: url(\"assets/pattern.png\"); }"),
            QStringLiteral("/themes/mine"), &problems);

        check(!cleaned.contains(QLatin1String("@import")), QStringLiteral("@import вырезан"));
        check(!cleaned.contains(QLatin1String("https://evil")), QStringLiteral("ссылка в сеть вырезана"));
        check(!cleaned.contains(QLatin1String("behavior")), QStringLiteral("behavior вырезан"));
        check(cleaned.contains(QStringLiteral("url(\"/themes/mine/assets/pattern.png\")")),
              QStringLiteral("assets/ превращён в путь внутри темы"));
        check(problems.size() >= 3, QStringLiteral("о проблемах сообщено (%1)").arg(problems.size()));

        QStringList oversizeProblems;
        const QString huge = sanitizeUserCss(QString(300 * 1024, QLatin1Char('a')), QString(),
                                             &oversizeProblems);
        check(huge.size() <= 256 * 1024, QStringLiteral("стиль обрезан до 256 КБ"));
        check(!oversizeProblems.isEmpty(), QStringLiteral("обрезка попала в отчёт"));
    }

    // --- сборка QSS ---------------------------------------------------------
    {
        const Theme theme = Theme::fromJson(themeJson(QStringLiteral("dark"), QStringLiteral("Dark"),
                                                      QStringLiteral("#0b0d13"), QStringLiteral("#ff5500")),
                                            QString(), false, nullptr);
        const ResolvedTokens tokens = resolveTokens(theme, AppearanceOverrides());
        const StyleSheet::BuildResult result = StyleSheet::build(theme, tokens, QString(), false);

        check(result.qss.contains(QLatin1String("#ff5500")), QStringLiteral("акцент есть в QSS"));
        check(result.qss.contains(QStringLiteral("dataUi=\"titlebar\"")),
              QStringLiteral("хук шапки есть в QSS"));
        check(result.qss.contains(QLatin1String("dataUi=\"themeTile\"")),
              QStringLiteral("хук карточки темы есть в QSS"));

        // Панель плеера GlassPanel рисует сама, но тема и свой CSS её перекрывают:
        // правило из theme.qss обязано доехать до итогового QSS с подставленным токеном.
        Theme cssTheme = theme;
        cssTheme.css = QStringLiteral("QWidget[dataUi=\"player\"] { border-color: @accent; }");
        const StyleSheet::BuildResult withThemeCss
            = StyleSheet::build(cssTheme, tokens, QString(), false);
        check(withThemeCss.qss.contains(
                  QStringLiteral("QWidget[dataUi=\"player\"] { border-color: #ff5500; }")),
              QStringLiteral("правило из theme.qss дошло до QSS с подставленным акцентом"));

        // Главная проверка шаблона: ни один @token не остался неподставленным.
        const QRegularExpression unresolved(QStringLiteral("@[a-z][a-z0-9-]*"));
        const QStringList leftovers = result.qss.split(QLatin1Char('\n')).filter(unresolved);
        check(leftovers.isEmpty(),
              QStringLiteral("все токены подставлены (осталось: %1)").arg(leftovers.size()));
        if (!leftovers.isEmpty())
            std::printf("       первый неподставленный: %s\n",
                        leftovers.first().toUtf8().constData());
    }

    // --- свой CSS поверх темы ----------------------------------------------
    {
        const Theme theme = Theme::fromJson(themeJson(QStringLiteral("dark"), QStringLiteral("Dark"),
                                                      QStringLiteral("#0b0d13"), QStringLiteral("#ff5500")),
                                            QString(), false, nullptr);
        const ResolvedTokens tokens = resolveTokens(theme, AppearanceOverrides());
        const QString custom = QStringLiteral("QLabel[role=\"title\"] { color: @accent; }");

        const StyleSheet::BuildResult with = StyleSheet::build(theme, tokens, custom, true);
        check(with.qss.contains(QStringLiteral("QLabel[role=\"title\"] { color: #ff5500; }")),
              QStringLiteral("свой CSS подставлен и главнее (акцент в правиле)"));
        check(with.qss.contains(QStringLiteral("/* --- свой CSS --- */")),
              QStringLiteral("свой CSS добавлен в конец"));

        const StyleSheet::BuildResult without = StyleSheet::build(theme, tokens, custom, false);
        check(!without.qss.contains(QStringLiteral("/* --- свой CSS --- */")),
              QStringLiteral("выключенный свой CSS не применяется"));
    }

    // --- пример своей темы из репозитория -----------------------------------
    // Тест охраняет и документацию: themes/example должен оставаться рабочей темой.
    {
        const QString exampleDir = QDir(QStringLiteral(FLACORE_REPO_DIR))
                                       .filePath(QStringLiteral("themes/example"));
        QString error;
        const Theme example = Theme::loadFromDir(exampleDir, false, &error);
        check(example.isValid(), QStringLiteral("пример themes/example разбирается: %1").arg(error));
        check(example.css.contains(QLatin1String("@accent")),
              QStringLiteral("в примере есть theme.qss с токенами"));

        const ResolvedTokens tokens = resolveTokens(example, AppearanceOverrides());
        const StyleSheet::BuildResult built = StyleSheet::build(example, tokens, QString(), false);
        check(!built.qss.contains(QLatin1String("@accent")),
              QStringLiteral("QSS примера собирается без неподставленных токенов"));
        check(built.problems.isEmpty(), QStringLiteral("пример не даёт замечаний при загрузке"));
    }

    // --- контраст встроенных тем (WCAG) -------------------------------------
    // Красота без читаемости не считается: проверяем не «на глаз», а по числам.
    {
        Q_INIT_RESOURCE(resources);
        QDir bundled(QStringLiteral(":/themes"));
        const QStringList ids = bundled.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        check(!ids.isEmpty(), QStringLiteral("встроенные темы доступны в ресурсах"));

        const auto linear = [](double channel) {
            return channel <= 0.03928 ? channel / 12.92
                                      : std::pow((channel + 0.055) / 1.055, 2.4);
        };
        const auto luminance = [&linear](const QColor &color) {
            return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF())
                   + 0.0722 * linear(color.blueF());
        };
        const auto contrast = [&luminance](const QColor &a, const QColor &b) {
            double first = luminance(a);
            double second = luminance(b);
            if (first < second)
                std::swap(first, second);
            return (first + 0.05) / (second + 0.05);
        };

        for (const QString &id : ids) {
            QString error;
            const Theme theme = Theme::loadFromDir(bundled.filePath(id), true, &error);
            if (!theme.isValid()) {
                check(false, QStringLiteral("тема %1 читается: %2").arg(id, error));
                continue;
            }

            const ResolvedTokens tokens = resolveTokens(theme, AppearanceOverrides());
            const auto tokenColor = [&tokens](const char *key) {
                return QColor(tokens.values.value(QString::fromLatin1(key)));
            };
            const QColor surface = tokenColor("panel-solid");
            const double textContrast = contrast(tokenColor("text"), tokenColor("bg-primary"));
            const double mutedContrast = contrast(tokenColor("text-muted"), surface);
            const double faintContrast = contrast(tokenColor("text-faint"), surface);
            const double dangerContrast = contrast(tokenColor("danger"), surface);
            const double accentContrast = contrast(tokenColor("accent-contrast"), tokenColor("accent"));

            check(textContrast >= 7.0,
                  QStringLiteral("«%1»: основной текст %2:1 (нужно ≥ 7)")
                      .arg(theme.name)
                      .arg(textContrast, 0, 'f', 1));
            check(mutedContrast >= 4.0,
                  QStringLiteral("«%1»: второстепенный текст %2:1 (нужно ≥ 4)")
                      .arg(theme.name)
                      .arg(mutedContrast, 0, 'f', 1));
            check(faintContrast >= 2.5,
                  QStringLiteral("«%1»: самый тихий текст %2:1 (нужно ≥ 2.5)")
                      .arg(theme.name)
                      .arg(faintContrast, 0, 'f', 1));
            check(dangerContrast >= 3.5,
                  QStringLiteral("«%1»: цвет ошибки %2:1 (нужно ≥ 3.5)")
                      .arg(theme.name)
                      .arg(dangerContrast, 0, 'f', 1));
            check(accentContrast >= 3.0,
                  QStringLiteral("«%1»: текст на акценте %2:1 (нужно ≥ 3)")
                      .arg(theme.name)
                      .arg(accentContrast, 0, 'f', 1));
        }
    }

    std::printf("\n%s\n", g_failures == 0 ? "ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ" : "ЕСТЬ ПРОВАЛЫ");
    std::printf("Провалено: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}