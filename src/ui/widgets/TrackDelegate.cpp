#include "TrackDelegate.h"

#include "../theme/ThemeManager.h"
#include "../theme/ThemeUtil.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace {

constexpr int kRowHeight = 54;
constexpr int kAccentBar = 3;
constexpr int kIndicatorWidth = 18;
constexpr int kDurationWidth = 54;

} // namespace

TrackDelegate::TrackDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    auto *view = qobject_cast<QAbstractItemView *>(parent);
    if (ThemeManager *manager = ThemeManager::instance()) {
        connect(manager, &ThemeManager::changed, this, [view]() {
            if (view && view->viewport())
                view->viewport()->update();
        });
    }
}

void TrackDelegate::setPlayingRow(int row)
{
    if (m_playing_row == row)
        return;
    m_playing_row = row;
    if (auto *view = qobject_cast<QAbstractItemView *>(parent())) {
        if (view->viewport())
            view->viewport()->update();
    }
}

QSize TrackDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    Q_UNUSED(option);
    return QSize(200, kRowHeight);
}

void TrackDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    const qreal radius = ThemeUtil::number(QStringLiteral("radius-sm"), 10);
    const QColor text = ThemeUtil::color(QStringLiteral("text"), QColor(0xf2, 0xf4, 0xf8));
    const QColor muted = ThemeUtil::color(QStringLiteral("text-muted"), text);
    const QColor faint = ThemeUtil::color(QStringLiteral("text-faint"), muted);
    const QColor accent = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    const QColor hover = ThemeUtil::color(QStringLiteral("hover-bg"), QColor(255, 255, 255, 16));
    const QColor selection = ThemeUtil::color(QStringLiteral("sel-bg"), QColor(255, 255, 255, 24));

    const bool isPlaying = index.row() == m_playing_row;
    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;

    const QRectF row = QRectF(opt.rect).adjusted(3, 2, -3, -2);

    if (selected || hovered) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(selected ? selection : hover);
        painter->drawRoundedRect(row, radius, radius);

        if (selected) {
            painter->setBrush(accent);
            painter->drawRoundedRect(QRectF(row.left() + 5, row.center().y() - 11, kAccentBar, 22),
                                     kAccentBar / 2.0, kAccentBar / 2.0);
        }
    }

    // Индикатор «играет сейчас»: три полоски разной высоты.
    if (isPlaying) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(accent);
        const qreal baseX = row.left() + 9;
        const qreal centerY = row.center().y();
        const qreal heights[3] = {8.0, 14.0, 5.0};
        for (int i = 0; i < 3; ++i) {
            const qreal barHeight = heights[i];
            painter->drawRoundedRect(QRectF(baseX + i * 4.0, centerY - barHeight / 2.0, 2.6,
                                            barHeight),
                                     1.3, 1.3);
        }
    }

    const qreal contentLeft = row.left() + (isPlaying ? kIndicatorWidth + 8 : 14);
    const qreal contentRight = row.right() - kDurationWidth - 8;
    const qreal available = qMax(40.0, contentRight - contentLeft);

    QString title = index.data(Qt::DisplayRole).toString();
    QString artist = index.data(ArtistRole).toString();
    const QString album = index.data(AlbumRole).toString();
    if (!album.isEmpty() && album != artist)
        artist = artist.isEmpty() ? album : artist + QStringLiteral("  ·  ") + album;

    QFont titleFont = opt.font;
    titleFont.setPointSizeF(opt.font.pointSizeF() * 1.05);
    titleFont.setWeight(isPlaying ? QFont::DemiBold : QFont::Medium);

    QFont metaFont = opt.font;
    metaFont.setPointSizeF(qMax(7.0, opt.font.pointSizeF() * 0.92));

    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics metaMetrics(metaFont);

    const qreal titleTop = row.center().y() - (artist.isEmpty() ? 0.0 : 10.0);
    painter->setFont(titleFont);
    painter->setPen(isPlaying ? accent : text);
    painter->drawText(QRectF(contentLeft, titleTop - titleMetrics.height() / 2.0, available,
                             titleMetrics.height()),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      titleMetrics.elidedText(title, Qt::ElideRight, int(available)));

    if (!artist.isEmpty()) {
        painter->setFont(metaFont);
        painter->setPen(muted);
        painter->drawText(QRectF(contentLeft, row.center().y() + 4.0, available,
                                 metaMetrics.height()),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          metaMetrics.elidedText(artist, Qt::ElideRight, int(available)));
    }

    const QString duration = index.data(DurationRole).toString();
    if (!duration.isEmpty()) {
        painter->setFont(metaFont);
        painter->setPen(faint);
        painter->drawText(QRectF(row.right() - kDurationWidth, row.top(), kDurationWidth - 6,
                                 row.height()),
                          Qt::AlignVCenter | Qt::AlignRight, duration);
    }

    painter->restore();
}