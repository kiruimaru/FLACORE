#include <QApplication>
#include <QStyleFactory>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("FLACORE"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    MainWindow window;
    window.show();

    return app.exec();
}
