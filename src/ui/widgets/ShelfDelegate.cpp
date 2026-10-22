#include "ShelfDelegate.h"

#include "core/online/CoverLoader.h"

#include "../theme/ThemeManager.h"
#include "../theme/ThemeUtil.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace {

constexpr int kCoverSide = 38;
constexpr int kDurationWidth = 54;

} // namespace

ShelfDelegate::ShelfDelegate(CoverLoader *covers, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_covers(covers)
{
    if (ThemeManager *manager = ThemeManager::instance()) {
        auto *view = qobject_cast<QAbstractItemView *>(parent);
        connect(manager, &ThemeManager::changed, this, [view]() {
            if (view && view->viewport())
                view->viewport()->update();
        });
    }
}

QSize ShelfDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    Q_UNUSED(option);
    return QSize(200, RowHeight);
}

void ShelfDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const qreal radius = ThemeUtil::number(QStringLiteral("radius-sm"), 10);
    const QColor text = ThemeUtil::color(QStringLiteral("text"), QColor(0xf2, 0xf4, 0xf8));
    const QColor muted = ThemeUtil::color(QStringLiteral("text-muted"), text);
    const QColor faint = ThemeUtil::color(QStringLiteral("text-faint"), muted);
    const QColor accent = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    const QColor hover = ThemeUtil::color(QStringLiteral("hover-bg"), QColor(255, 255, 255, 16));
    const QColor selection = ThemeUtil::color(QStringLiteral("sel-bg"), QColor(255, 255, 255, 24));

    // Подсказка вместо строк: только текст по центру, без обложки и длительности.
    if (index.data(HintRole).toBool()) {
        const QFontMetrics metrics(opt.font);
        painter->setFont(opt.font);
        painter->setPen(faint);
        painter->drawText(QRectF(opt.rect).adjusted(10, 0, -10, 0), Qt::AlignCenter,
                          metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                             Qt::ElideRight, opt.rect.width() - 20));
        painter->restore();
        return;
    }

    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;
    const QRectF row = QRectF(opt.rect).adjusted(3, 2, -3, -2);

    if (selected || hovered) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(selected ? selection : hover);
        painter->drawRoundedRect(row, radius, radius);
    }

    // --- миниатюра обложки --------------------------------------------------
    const QString coverUrl = index.data(CoverUrlRole).toString();
    const QRectF coverBox(row.left() + 8, row.center().y() - kCoverSide / 2.0, kCoverSide,
                          kCoverSide);

    QPainterPath coverPath;
    coverPath.addRoundedRect(coverBox, 8, 8);
    painter->setClipPath(coverPath);

    const QPixmap cover = m_covers ? m_covers->cached(coverUrl) : QPixmap();
    if (cover.isNull()) {
        QLinearGradient placeholder(coverBox.topLeft(), coverBox.bottomRight());
        QColor start = accent;
        start.setAlphaF(0.5);
        placeholder.setColorAt(0.0, start);
        placeholder.setColorAt(1.0, ThemeUtil::color(QStringLiteral("panel-solid"), Qt::black));
        painter->fillRect(coverBox, placeholder);

        if (m_covers && !coverUrl.isEmpty()) {
            m_covers->request(coverUrl);   // придёт сигнал loaded — список перерисуется
        }
    } else {
        const QImage scaled = cover.toImage().scaled(int(coverBox.width()), int(coverBox.height()),
                                                     Qt::KeepAspectRatioByExpanding,
                                                     Qt::SmoothTransformation);
        painter->drawImage(coverBox.topLeft(), scaled);
    }
    painter->setClipping(false);

    // --- тексты -------------------------------------------------------------
    const QString duration = index.data(DurationRole).toString();
    const QString source = index.data(SourceRole).toString();
    const qreal contentLeft = coverBox.right() + 12;
    const qreal contentRight = row.right() - kDurationWidth - 8;
    const qreal available = qMax(60.0, contentRight - contentLeft);

    QFont titleFont = opt.font;
    titleFont.setWeight(QFont::Medium);
    QFont metaFont = opt.font;
    metaFont.setPointSizeF(qMax(7.0, opt.font.pointSizeF() * 0.9));

    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics metaMetrics(metaFont);

    painter->setFont(titleFont);
    painter->setPen(text);
    painter->drawText(QRectF(contentLeft, row.center().y() - 15.0, available,
                             titleMetrics.height()),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      titleMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                              Qt::ElideRight, int(available)));

    QString subtitle = index.data(ArtistRole).toString();
    if (!source.isEmpty())
        subtitle += subtitle.isEmpty() ? source : QStringLiteral("  ·  ") + source;

    painter->setFont(metaFont);
    painter->setPen(muted);
    painter->drawText(QRectF(contentLeft, row.center().y() + 1.0, available, metaMetrics.height()),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      metaMetrics.elidedText(subtitle, Qt::ElideRight, int(available)));

    if (!duration.isEmpty()) {
        painter->setPen(faint);
        painter->drawText(QRectF(row.right() - kDurationWidth, row.top(), kDurationWidth - 6,
                                 row.height()),
                          Qt::AlignVCenter | Qt::AlignRight, duration);
    }

    painter->restore();
}