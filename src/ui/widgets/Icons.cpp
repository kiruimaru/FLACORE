#include "Icons.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

namespace {

// Все иконки нарисованы на условной сетке 24x24 и масштабируются под размер.
constexpr qreal kGrid = 24.0;

QPainterPath triangle(const QPointF &a, const QPointF &b, const QPointF &c)
{
    QPainterPath path;
    path.moveTo(a);
    path.lineTo(b);
    path.lineTo(c);
    path.closeSubpath();
    return path;
}

void paintSpeaker(QPainter &painter)
{
    QPainterPath body;
    body.moveTo(3.5, 9.0);
    body.lineTo(7.5, 9.0);
    body.lineTo(12.5, 4.5);
    body.lineTo(12.5, 19.5);
    body.lineTo(7.5, 15.0);
    body.lineTo(3.5, 15.0);
    body.closeSubpath();
    painter.drawPath(body);

    painter.drawArc(QRectF(11.0, 6.5, 9.0, 11.0), -60 * 16, 120 * 16);
}

void paintGear(QPainter &painter)
{
    painter.drawEllipse(QPointF(12, 12), 7.2, 7.2);
    painter.drawEllipse(QPointF(12, 12), 2.6, 2.6);
    painter.save();
    painter.translate(12, 12);
    for (int i = 0; i < 8; ++i) {
        painter.drawLine(QPointF(0, -7.2), QPointF(0, -10.2));
        painter.rotate(45);
    }
    painter.restore();
}

void paintNote(QPainter &painter)
{
    painter.drawEllipse(QPointF(8.6, 17.0), 3.1, 2.6);
    painter.drawEllipse(QPointF(17.0, 15.0), 3.1, 2.6);
    painter.drawLine(QPointF(11.7, 17.0), QPointF(11.7, 6.4));
    painter.drawLine(QPointF(20.1, 15.0), QPointF(20.1, 4.6));
    painter.drawLine(QPointF(11.7, 6.4), QPointF(20.1, 4.6));
}

} // namespace

namespace Icons {

const char *name(Kind kind)
{
    switch (kind) {
    case Kind::Play: return "play";
    case Kind::Pause: return "pause";
    case Kind::Stop: return "stop";
    case Kind::Previous: return "previous";
    case Kind::Next: return "next";
    case Kind::Volume: return "volume";
    case Kind::VolumeMuted: return "volume-muted";
    case Kind::Shuffle: return "shuffle";
    case Kind::Repeat: return "repeat";
    case Kind::Folder: return "folder";
    case Kind::Plus: return "plus";
    case Kind::Search: return "search";
    case Kind::Music: return "music";
    case Kind::Close: return "close";
    case Kind::Minimize: return "minimize";
    case Kind::Maximize: return "maximize";
    case Kind::Restore: return "restore";
    case Kind::Palette: return "palette";
    case Kind::Settings: return "settings";
    case Kind::Trash: return "trash";
    case Kind::Info: return "info";
    case Kind::Refresh: return "refresh";
    case Kind::Download: return "download";
    case Kind::YouTube: return "youtube";
    case Kind::Library: return "library";
    case Kind::Check: return "check";
    case Kind::Menu: return "menu";
    case Kind::Link: return "link";
    case Kind::Image: return "image";
    case Kind::Reset: return "reset";
    case Kind::Discover: return "discover";
    }
    return "unknown";
}

QPixmap pixmap(Kind kind, const QColor &color, int size, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0)
        devicePixelRatio = qGuiApp ? qGuiApp->devicePixelRatio() : 1.0;

    const int pixelSize = qMax(8, size);
    QPixmap pixmap(QSize(int(pixelSize * devicePixelRatio), int(pixelSize * devicePixelRatio)));
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setWindow(QRect(0, 0, int(kGrid), int(kGrid)));
    painter.setViewport(QRect(0, 0, pixelSize, pixelSize));

    QPen pen(color);
    pen.setWidthF(1.9);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (kind) {
    case Kind::Play:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPath(triangle(QPointF(6.5, 4.5), QPointF(19.5, 12), QPointF(6.5, 19.5)));
        break;
    case Kind::Pause:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(6.5, 4.5, 4.0, 15.0), 1.4, 1.4);
        painter.drawRoundedRect(QRectF(13.5, 4.5, 4.0, 15.0), 1.4, 1.4);
        break;
    case Kind::Stop:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(6.5, 6.5, 11.0, 11.0), 2.1, 2.1);
        break;
    case Kind::Previous:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPath(triangle(QPointF(18.5, 5.0), QPointF(18.5, 19.0), QPointF(7.5, 12.0)));
        painter.drawRoundedRect(QRectF(4.0, 5.0, 2.8, 14.0), 1.2, 1.2);
        break;
    case Kind::Next:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPath(triangle(QPointF(5.5, 5.0), QPointF(5.5, 19.0), QPointF(16.5, 12.0)));
        painter.drawRoundedRect(QRectF(17.2, 5.0, 2.8, 14.0), 1.2, 1.2);
        break;
    case Kind::Volume:
        painter.setBrush(color);
        paintSpeaker(painter);
        break;
    case Kind::VolumeMuted: {
        painter.setBrush(color);
        paintSpeaker(painter);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(15.0, 8.5), QPointF(21.0, 15.5));
        painter.drawLine(QPointF(21.0, 8.5), QPointF(15.0, 15.5));
        break;
    }
    case Kind::Shuffle:
        painter.drawPath(QPainterPath());
        painter.drawLine(QPointF(4, 7), QPointF(9, 7));
        painter.drawLine(QPointF(9, 7), QPointF(15, 17));
        painter.drawLine(QPointF(15, 17), QPointF(20, 17));
        painter.drawLine(QPointF(4, 17), QPointF(9, 17));
        painter.drawLine(QPointF(9, 17), QPointF(15, 7));
        painter.drawLine(QPointF(15, 7), QPointF(20, 7));
        painter.drawLine(QPointF(17, 14), QPointF(20, 17));
        painter.drawLine(QPointF(17, 20), QPointF(20, 17));
        painter.drawLine(QPointF(17, 4), QPointF(20, 7));
        painter.drawLine(QPointF(17, 10), QPointF(20, 7));
        break;
    case Kind::Repeat:
        painter.drawArc(QRectF(4.5, 5.5, 15.0, 13.0), 90 * 16, 250 * 16);
        painter.drawLine(QPointF(18.5, 4.0), QPointF(21.0, 8.0));
        painter.drawLine(QPointF(18.5, 12.0), QPointF(21.0, 8.0));
        break;
    case Kind::Folder:
        painter.drawPath([]{
            QPainterPath path;
            path.moveTo(3.5, 6.5);
            path.lineTo(9.5, 6.5);
            path.lineTo(11.5, 9.0);
            path.lineTo(20.5, 9.0);
            path.lineTo(20.5, 18.0);
            path.lineTo(3.5, 18.0);
            path.closeSubpath();
            return path;
        }());
        break;
    case Kind::Plus:
        painter.drawLine(QPointF(12, 5.5), QPointF(12, 18.5));
        painter.drawLine(QPointF(5.5, 12), QPointF(18.5, 12));
        break;
    case Kind::Search:
        painter.drawEllipse(QPointF(10.5, 10.5), 5.6, 5.6);
        painter.drawLine(QPointF(14.8, 14.8), QPointF(19.5, 19.5));
        break;
    case Kind::Music:
        paintNote(painter);
        break;
    case Kind::Close:
        painter.drawLine(QPointF(6.5, 6.5), QPointF(17.5, 17.5));
        painter.drawLine(QPointF(17.5, 6.5), QPointF(6.5, 17.5));
        break;
    case Kind::Minimize:
        painter.drawLine(QPointF(6.0, 12.0), QPointF(18.0, 12.0));
        break;
    case Kind::Maximize:
        painter.drawRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 2.0, 2.0);
        break;
    case Kind::Restore:
        painter.drawRoundedRect(QRectF(4.5, 8.0, 11.0, 11.0), 2.0, 2.0);
        painter.drawLine(QPointF(8.5, 5.0), QPointF(19.5, 5.0));
        painter.drawLine(QPointF(19.5, 5.0), QPointF(19.5, 16.0));
        break;
    case Kind::Palette: {
        painter.drawEllipse(QPointF(12, 12), 8.0, 8.0);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(8.6, 10.0), 1.5, 1.5);
        painter.drawEllipse(QPointF(12.0, 7.6), 1.5, 1.5);
        painter.drawEllipse(QPointF(15.4, 10.0), 1.5, 1.5);
        painter.drawEllipse(QPointF(9.4, 15.0), 1.5, 1.5);
        break;
    }
    case Kind::Settings:
        paintGear(painter);
        break;
    case Kind::Trash:
        painter.drawLine(QPointF(5.0, 7.0), QPointF(19.0, 7.0));
        painter.drawLine(QPointF(9.5, 7.0), QPointF(9.5, 5.0));
        painter.drawLine(QPointF(14.5, 7.0), QPointF(14.5, 5.0));
        painter.drawLine(QPointF(9.5, 5.0), QPointF(14.5, 5.0));
        painter.drawRoundedRect(QRectF(6.5, 7.0, 11.0, 13.5), 1.8, 1.8);
        painter.drawLine(QPointF(10.5, 10.5), QPointF(10.5, 17.0));
        painter.drawLine(QPointF(13.5, 10.5), QPointF(13.5, 17.0));
        break;
    case Kind::Info:
        painter.drawEllipse(QPointF(12, 12), 8.0, 8.0);
        painter.drawLine(QPointF(12, 11.0), QPointF(12, 16.5));
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(12, 8.0), 1.3, 1.3);
        break;
    case Kind::Refresh:
        painter.drawArc(QRectF(4.5, 4.5, 15.0, 15.0), 40 * 16, 280 * 16);
        painter.drawLine(QPointF(17.5, 3.5), QPointF(20.0, 7.5));
        painter.drawLine(QPointF(17.5, 11.5), QPointF(20.0, 7.5));
        break;
    case Kind::Download:
        painter.drawLine(QPointF(12, 4.5), QPointF(12, 15.0));
        painter.drawLine(QPointF(7.5, 10.5), QPointF(12, 15.0));
        painter.drawLine(QPointF(16.5, 10.5), QPointF(12, 15.0));
        painter.drawLine(QPointF(5.0, 19.0), QPointF(19.0, 19.0));
        break;
    case Kind::YouTube:
        painter.drawRoundedRect(QRectF(3.0, 6.0, 18.0, 12.0), 3.2, 3.2);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPath(triangle(QPointF(10.0, 9.0), QPointF(16.0, 12), QPointF(10.0, 15.0)));
        break;
    case Kind::Library:
        painter.drawLine(QPointF(5.0, 6.0), QPointF(5.0, 18.0));
        painter.drawLine(QPointF(9.5, 4.5), QPointF(9.5, 19.5));
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(13.5, 6.0, 3.0, 12.0), 1.2, 1.2);
        painter.drawRoundedRect(QRectF(18.0, 8.5, 3.0, 9.5), 1.2, 1.2);
        break;
    case Kind::Check:
        painter.drawLine(QPointF(5.5, 12.5), QPointF(10.0, 17.0));
        painter.drawLine(QPointF(10.0, 17.0), QPointF(18.5, 7.0));
        break;
    case Kind::Menu:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(12, 5.5), 1.6, 1.6);
        painter.drawEllipse(QPointF(12, 12.0), 1.6, 1.6);
        painter.drawEllipse(QPointF(12, 18.5), 1.6, 1.6);
        break;
    case Kind::Link:
        painter.drawArc(QRectF(3.0, 8.0, 12.0, 8.0), 45 * 16, 200 * 16);
        painter.drawArc(QRectF(9.0, 8.0, 12.0, 8.0), 225 * 16, 200 * 16);
        painter.drawLine(QPointF(9.0, 12.0), QPointF(15.0, 12.0));
        break;
    case Kind::Image:
        painter.drawRoundedRect(QRectF(3.5, 5.0, 17.0, 14.0), 2.4, 2.4);
        painter.drawLine(QPointF(4.5, 16.0), QPointF(10.0, 11.0));
        painter.drawLine(QPointF(10.0, 11.0), QPointF(14.5, 15.5));
        painter.drawLine(QPointF(14.5, 15.5), QPointF(17.0, 13.0));
        painter.drawLine(QPointF(17.0, 13.0), QPointF(20.0, 16.0));
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(9.0, 9.0), 1.5, 1.5);
        break;
    case Kind::Reset:
            painter.drawArc(QRectF(4.5, 4.5, 15.0, 15.0), 130 * 16, 260 * 16);
            painter.drawLine(QPointF(3.0, 4.0), QPointF(7.5, 6.0));
            painter.drawLine(QPointF(3.0, 4.0), QPointF(6.0, 1.5));
            break;
        case Kind::Discover: {
            // Компас: «живая подборка», обзор чартов.
            painter.drawEllipse(QPointF(12, 12), 8.2, 8.2);
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            QPainterPath needle;
            needle.moveTo(15.6, 8.4);
            needle.lineTo(10.6, 10.6);
            needle.lineTo(8.4, 15.6);
            needle.lineTo(13.4, 13.4);
            needle.closeSubpath();
            painter.drawPath(needle);
            break;
        }
        }

    painter.end();
    return pixmap;
}

QIcon icon(Kind kind, const QColor &color, int size)
{
    QIcon result;
    result.addPixmap(pixmap(kind, color, size, 1.0));
    result.addPixmap(pixmap(kind, color, size, 2.0), QIcon::Normal, QIcon::Off);
    return result;
}

} // namespace Icons