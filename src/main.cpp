#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>

#include "ui/MainWindow.h"
#include "ui/theme/ThemeManager.h"
#include "ui/widgets/Icons.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QSettings и пути тем зависят от этих имён — задаём до первого обращения.
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("flacore.app"));
    QCoreApplication::setApplicationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FLACORE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("FLACORE — проигрыватель FLAC/MP3 с загрузкой через yt-dlp"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption safeMode(
        QStringLiteral("no-custom-css"),
        QStringLiteral("Запустить без «своего CSS» и без темы каталога (аварийный режим)"));
    parser.addOption(safeMode);
    parser.process(app);

    ThemeManager::setSafeMode(parser.isSet(safeMode));

    // Ресурсы лежат в статической библиотеке — инициализируем их вручную.
    Q_INIT_RESOURCE(resources);

    ThemeManager themeManager;
    themeManager.apply();

    app.setWindowIcon(QIcon(Icons::pixmap(Icons::Kind::Music,
                                          themeManager.tokens().accent, 64)));

    MainWindow window;
    window.show();

    return app.exec();
}