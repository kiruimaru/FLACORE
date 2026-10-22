#include "ThemeUtil.h"

#include "ThemeManager.h"

namespace ThemeUtil {

QColor color(const QString &token, const QColor &fallback)
{
    const ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return fallback;
    const QColor parsed(manager->tokens().values.value(token));
    return parsed.isValid() ? parsed : fallback;
}

int number(const QString &token, int fallback)
{
    const ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return fallback;
    bool ok = false;
    const int value = manager->tokens().values.value(token).toInt(&ok);
    return ok ? value : fallback;
}

qreal number(const QString &token, qreal fallback)
{
    const ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return fallback;
    bool ok = false;
    const qreal value = manager->tokens().values.value(token).toDouble(&ok);
    return ok ? value : fallback;
}

QString text(const QString &token, const QString &fallback)
{
    const ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return fallback;
    const QString value = manager->tokens().values.value(token);
    return value.isEmpty() ? fallback : value;
}

bool isLight()
{
    const ThemeManager *manager = ThemeManager::instance();
    return manager && manager->tokens().isLight;
}

} // namespace ThemeUtil