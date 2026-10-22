#include "ThemeTile.h"

#include "../widgets/Icons.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

ThemeTile::ThemeTile(const Theme &theme, QWidget *parent)
    : QPushButton(parent)
    , m_theme(theme)
{
    setProperty("dataUi", "themeTile");
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMinimumHeight(96);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("%1 — %2\n%3\nv%4 · %5%6")
                   .arg(theme.name, theme.description(), theme.dirPath, theme.version,
                        theme.authorName,
                        theme.builtin ? QStringLiteral(" · встроенная") : QStringLiteral("")));
}

void ThemeTile::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    setProperty("active", active ? QStringLiteral("true") : QStringLiteral("false"));
    style()->unpolish(this);
    style()->polish(this);
    update();
}

QSize ThemeTile::sizeHint() const
{
    return QSize(200, 96);
}

void ThemeTile::paintEvent(QPaintEvent *)
{
    // Сначала QSS-фон и рамка (dataUi="themeTile"), затем образцы и текст.
    QStyleOptionButton option;
    initStyleOption(&option);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect content = rect().adjusted(12, 12, -12, -12);
    const QRect swatch(content.left(), content.top(), content.width(), 30);

    const QColor background(m_theme.background);
    const QColor accent(m_theme.accent);
    const QColor panel = background.lighter(160);

    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(swatch, 7, 7);

    // Полоса «стекла» и акцентная точка — быстрый образ темы.
    painter.setBrush(panel);
    painter.drawRoundedRect(swatch.adjusted(6, 6, -6, -6), 5, 5);
    painter.setBrush(accent);
    painter.drawEllipse(QPointF(swatch.left() + 22, swatch.center().y() - 1), 6, 6);
    QColor accentSoft = accent;
    accentSoft.setAlphaF(0.45);
    painter.setBrush(accentSoft);
    painter.drawRoundedRect(QRectF(swatch.left() + 34, swatch.center().y() - 4, 52, 8), 4, 4);

    const QColor text = QColor(m_theme.background).lightness() > 140 ? QColor(0x15, 0x17, 0x1d)
                                                                    : QColor(0xf2, 0xf4, 0xf8);
    QColor muted = text;
    muted.setAlphaF(0.65);

    QFont nameFont = font();
    nameFont.setWeight(QFont::DemiBold);
    nameFont.setPointSizeF(font().pointSizeF() * 1.02);
    painter.setFont(nameFont);
    painter.setPen(text);

    const QRect nameRect(content.left(), swatch.bottom() + 10, content.width() - 22,
                         QFontMetrics(nameFont).height());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetrics(nameFont).elidedText(m_theme.name, Qt::ElideRight,
                                                       nameRect.width()));

    QFont metaFont = font();
    metaFont.setPointSizeF(qMax(7.0, font().pointSizeF() * 0.9));
    painter.setFont(metaFont);
    painter.setPen(muted);
    const QRect metaRect(content.left(), nameRect.bottom() + 2, content.width(), 
                         QFontMetrics(metaFont).height());
    painter.drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetrics(metaFont).elidedText(m_theme.description(), Qt::ElideRight,
                                                       metaRect.width()));

    if (m_active) {
        const QColor accentDot = accent;
        const int badge = 18;
        const QRect badgeRect(content.right() - badge, content.top(), badge, badge);
        painter.setBrush(accentDot);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(badgeRect.center()), badge / 2.0, badge / 2.0);

        const QPixmap check = Icons::pixmap(Icons::Kind::Check, QColor(0xff, 0xff, 0xff), 11);
        const QRect checkRect(int(badgeRect.center().x()) - 5, int(badgeRect.center().y()) - 6, 11,
                              11);
        painter.drawPixmap(checkRect, check);
    }
}