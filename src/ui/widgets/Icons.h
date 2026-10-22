#ifndef ICONS_H
#define ICONS_H

#include <QColor>
#include <QIcon>
#include <QPixmap>

class QPainter;

/**
 * Иконки рисуются вектором прямо в QPixmap — без файлов и без Qt::Svg.
 * Цвет берётся из темы, поэтому иконки перекрашиваются вместе с интерфейсом.
 */
namespace Icons {

enum class Kind {
    Play,
    Pause,
    Stop,
    Previous,
    Next,
    Volume,
    VolumeMuted,
    Shuffle,
    Repeat,
    Folder,
    Plus,
    Search,
    Music,
    Close,
    Minimize,
    Maximize,
    Restore,
    Palette,
    Settings,
    Trash,
    Info,
    Refresh,
    Download,
    YouTube,
    Library,
        Check,
        Menu,
        Link,
        Image,
        Reset,
        Discover,
    };

QPixmap pixmap(Kind kind, const QColor &color, int size, qreal devicePixelRatio = 0.0);
QIcon icon(Kind kind, const QColor &color, int size);
const char *name(Kind kind);

} // namespace Icons

#endif // ICONS_H