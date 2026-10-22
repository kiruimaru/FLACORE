#ifndef THEMEUTIL_H
#define THEMEUTIL_H

#include <QColor>
#include <QString>

/**
 * Быстрый доступ к токенам текущей темы из виджетов, которые рисуют себя сами
 * (QSS их не красит). Если ThemeManager ещё не создан, отдаются значения-заглушки.
 */
namespace ThemeUtil {

QColor color(const QString &token, const QColor &fallback = QColor());
int number(const QString &token, int fallback);
qreal number(const QString &token, qreal fallback);
QString text(const QString &token, const QString &fallback = QString());
bool isLight();

} // namespace ThemeUtil

#endif // THEMEUTIL_H