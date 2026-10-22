#ifndef STYLESHEET_H
#define STYLESHEET_H

#include "Theme.h"

#include <QString>
#include <QStringList>

/**
 * Сборка итогового QSS: базовый шаблон (токены @…) + theme.qss темы + «свой CSS»
 * пользователя. Всё проходит через аварийный разбор, ошибки возвращаются списком.
 */
namespace StyleSheet {

struct BuildResult {
    QString qss;
    QStringList problems;
};

/// Базовые стили приложения (шаблон с токенами) — то, что theme.qss перекрывает.
QString baseTemplate();

BuildResult build(const Theme &theme, const ResolvedTokens &tokens, const QString &userCss,
                  bool customCssEnabled);

} // namespace StyleSheet

#endif // STYLESHEET_H