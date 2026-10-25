#include "GlassWidgets.h"

#include "../theme/ThemeManager.h"
#include "../theme/ThemeUtil.h"

#include <QEnterEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStyle>
#include <QStyleOption>

namespace {

QSize squareSize(int size)
{
    return QSize(size, size);
}

} // namespace

// --- GlassPanel -------------------------------------------------------------

GlassPanel::GlassPanel(QWidget *parent, bool strong)
    : QWidget(parent)
    , m_strong(strong)
{
    // Свой фон рисуем сами, но при этом слушаем и QSS: правило из theme.qss или
    // из «своего CSS» (QWidget[dataUi="card"]) должно побеждать оформление по умолчанию.
    setAttribute(Qt::WA_StyledBackground, true);
    if (ThemeManager *manager = ThemeManager::instance())
        connect(manager, &ThemeManager::changed, this, qOverload<>(&QWidget::update));
}

void GlassPanel::setStrong(bool strong)
{
    m_strong = strong;
    update();
}

void GlassPanel::setInteractive(bool interactive)
{
    m_interactive = interactive;
    setAttribute(Qt::WA_Hover, interactive);
    update();
}

void GlassPanel::setRadius(int radius)
{
    m_radius = radius;
    update();
}

void GlassPanel::setShadowEnabled(bool enabled)
{
    m_shadow = enabled;
    update();
}

void GlassPanel::setHook(const QString &dataUi)
{
    setProperty("dataUi", dataUi);
    style()->unpolish(this);
    style()->polish(this);
}

void GlassPanel::enterEvent(QEnterEvent *event)
{
    if (m_interactive) {
        m_hovered = true;
        update();
    }
    QWidget::enterEvent(event);
}

void GlassPanel::leaveEvent(QEvent *event)
{
    if (m_interactive) {
        m_hovered = false;
        update();
    }
    QWidget::leaveEvent(event);
}

void GlassPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal radius = m_radius >= 0 ? m_radius : ThemeUtil::number(QStringLiteral("radius"), 16);

    // Issue #7: учитываем glass-alpha для корректной прозрачности стеклянных панелей.
    // Токен glass-alpha (0.0..1.0) масштабирует непрозрачность заливки.
    const qreal glassAlpha = ThemeUtil::number(QStringLiteral("glass-alpha"), qreal(0.55));

    QColor fill = m_strong ? ThemeUtil::color(QStringLiteral("panel-strong"))
                           : ThemeUtil::color(QStringLiteral("panel"));
    // Применяем glass-alpha: масштабируем альфа-канал уже полупрозрачного fill.
    fill.setAlphaF(qBound(0.0, fill.alphaF() * (0.5 + glassAlpha * 0.5), 1.0));

    QColor border = ThemeUtil::color(QStringLiteral("border"));
    if (m_hovered)
        border = ThemeUtil::color(QStringLiteral("border-strong"), border);

    QRectF panel = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    if (m_shadow) {
        // Мягкая тень: несколько концентрических скруглений с нарастающей прозрачностью.
        const QColor shadow = ThemeUtil::color(QStringLiteral("shadow"), QColor(0, 0, 0, 120));
        for (int i = 3; i >= 1; --i) {
            QColor layer = shadow;
            layer.setAlphaF(qBound(0.0, shadow.alphaF() / (i * 2.4), 1.0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(layer);
            painter.drawRoundedRect(panel.adjusted(-i, -i, i, i), radius + i, radius + i);
        }
    }

    QPainterPath path;
    path.addRoundedRect(panel, radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawPath(path);

    // Блик сверху — «стекло». Интенсивность масштабируется с glass-alpha (Issue #7).
    QLinearGradient highlight(panel.topLeft(), panel.bottomLeft());
    const int highlightAlpha = ThemeUtil::isLight()
        ? int(140 * glassAlpha)
        : int(qBound(0.0, 26 * glassAlpha * 2.0, 80.0));
    const QColor top = QColor(255, 255, 255, highlightAlpha);
    highlight.setColorAt(0.0, top);
    highlight.setColorAt(1.0, Qt::transparent);
    painter.setBrush(highlight);
    painter.drawPath(path);

    painter.setBrush(Qt::NoBrush);
    QPen pen(border);
    pen.setWidthF(1.0);
    painter.setPen(pen);
    painter.drawPath(path);

    // QSS поверх своего оформления: если тема или пользователь задали фон/рамку
    // для этого хука — их правила видны.
    QStyleOption styleOption;
    styleOption.initFrom(this);
    painter.setBrush(Qt::NoBrush);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}

// --- CoverArt ---------------------------------------------------------------

CoverArt::CoverArt(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(48, 48);
    if (ThemeManager *manager = ThemeManager::instance())
        connect(manager, &ThemeManager::changed, this, qOverload<>(&QWidget::update));
}

void CoverArt::setImage(const QImage &image)
{
    m_image = image;
    update();
}

void CoverArt::clearImage()
{
    m_image = QImage();
    update();
}

void CoverArt::setRadius(int radius)
{
    m_radius = radius;
    update();
}

void CoverArt::setPlaceholderScale(qreal scale)
{
    m_placeholderScale = qBound(0.2, scale, 0.9);
    update();
}

QSize CoverArt::sizeHint() const
{
    return QSize(56, 56);
}

void CoverArt::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal radius = m_radius >= 0 ? m_radius : ThemeUtil::number(QStringLiteral("radius-sm"), 10);
    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    QPainterPath path;
    path.addRoundedRect(box, radius, radius);
    painter.setClipPath(path);

    if (!m_image.isNull()) {
        const QImage scaled = m_image.scaled(box.size().toSize(), Qt::KeepAspectRatioByExpanding,
                                             Qt::SmoothTransformation);
        const QRect crop((scaled.width() - int(box.width())) / 2,
                         (scaled.height() - int(box.height())) / 2, int(box.width()),
                         int(box.height()));
        painter.drawImage(box.topLeft(), scaled.copy(crop));
    } else {
        const QColor accent = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
        const QColor base = ThemeUtil::color(QStringLiteral("bg-elevated"), QColor(0x16, 0x18, 0x20));
        QLinearGradient gradient(box.topLeft(), box.bottomRight());
        QColor start = accent;
        start.setAlphaF(ThemeUtil::isLight() ? 0.55 : 0.75);
        gradient.setColorAt(0.0, start);
        gradient.setColorAt(1.0, base);
        painter.fillRect(box, gradient);

        const int glyph = int(qMin(box.width(), box.height()) * m_placeholderScale);
        const QPixmap icon = Icons::pixmap(Icons::Kind::Music,
                                           ThemeUtil::color(QStringLiteral("accent-contrast"),
                                                            QColor(255, 255, 255)),
                                           glyph);
        const QPointF centered(box.center().x() - glyph / 2.0, box.center().y() - glyph / 2.0);
        painter.setOpacity(0.85);
        painter.drawPixmap(centered, icon);
        painter.setOpacity(1.0);
    }

    painter.setClipping(false);
    QPen pen(QColor(255, 255, 255, ThemeUtil::isLight() ? 40 : 26));
    pen.setWidthF(1.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, radius, radius);
}

// --- SeekBar ----------------------------------------------------------------

SeekBar::SeekBar(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(22);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (ThemeManager *manager = ThemeManager::instance())
        connect(manager, &ThemeManager::changed, this, qOverload<>(&QWidget::update));
}

void SeekBar::setRange(int minimum, int maximum)
{
    m_minimum = minimum;
    m_maximum = qMax(minimum, maximum);
    setValue(qBound(m_minimum, m_value, m_maximum));
    update();
}

void SeekBar::setValue(int value)
{
    const int bounded = qBound(m_minimum, value, m_maximum);
    if (bounded == m_value)
        return;
    m_value = bounded;
    update();
}

QSize SeekBar::sizeHint() const
{
    return QSize(240, 22);
}

QRectF SeekBar::trackRect() const
{
    return QRectF(0, height() / 2.0 - 2.5, width(), 5.0);
}

qreal SeekBar::xForPosition(int position) const
{
    if (m_maximum <= m_minimum)
        return 0.0;
    const qreal ratio = qreal(position - m_minimum) / qreal(m_maximum - m_minimum);
    return ratio * width();
}

int SeekBar::positionFor(qreal x) const
{
    if (width() <= 0 || m_maximum <= m_minimum)
        return m_minimum;
    const qreal ratio = qBound(0.0, x / width(), 1.0);
    return m_minimum + int(qRound(ratio * (m_maximum - m_minimum)));
}

void SeekBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor trackBase = ThemeUtil::color(QStringLiteral("panel-strong"), QColor(255, 255, 255, 24));
    const QColor accent = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    const QColor accentHover = ThemeUtil::color(QStringLiteral("accent-hover"), accent);
    const QColor glow = ThemeUtil::color(QStringLiteral("accent-glow"), accent);
    const QColor text = ThemeUtil::color(QStringLiteral("text"), Qt::white);

    const bool active = isEnabled();
    const qreal trackHeight = (m_hovered || m_dragging) ? 8.0 : 6.0;
    const QRectF bar(0, height() / 2.0 - trackHeight / 2.0, width(), trackHeight);

    painter.setPen(Qt::NoPen);
    QColor trackColor = trackBase;
    if (!active)
        trackColor.setAlphaF(trackColor.alphaF() * 0.6);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(bar, trackHeight / 2.0, trackHeight / 2.0);

    if (m_maximum <= m_minimum)
        return;

    const qreal x = xForPosition(m_value);
    QRectF progress = bar;
    progress.setWidth(x);

    if (active && progress.width() > 1.0) {
        // Свечение под прогрессом.
        QColor glowColor = glow;
        glowColor.setAlphaF(glowColor.alphaF() * 0.35);
        painter.setBrush(glowColor);
        painter.drawRoundedRect(progress.adjusted(-1.0, -3.0, 1.0, 3.0), 6, 6);

        QLinearGradient gradient(progress.topLeft(), progress.topRight());
        gradient.setColorAt(0.0, accentHover);
        gradient.setColorAt(1.0, accent);
        painter.setBrush(active ? gradient : QBrush(accent));
        painter.drawRoundedRect(progress, trackHeight / 2.0, trackHeight / 2.0);
    }

    if (m_hovered && !m_dragging && m_hoverX >= 0.0) {
        const int preview = positionFor(m_hoverX);
        QColor previewColor = text;
        previewColor.setAlphaF(0.45);
        painter.setBrush(previewColor);
        painter.drawEllipse(QPointF(xForPosition(preview), height() / 2.0), 2.6, 2.6);
    }

    if (active) {
        const qreal handleRadius = m_dragging ? 9.0 : (m_hovered ? 8.0 : 6.5);
        QColor handleColor = text;
        if (!m_hovered && !m_dragging)
            handleColor.setAlphaF(0.85);
        painter.setBrush(handleColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x, height() / 2.0), handleRadius, handleRadius);
    }
}

void SeekBar::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled() || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_dragging = true;
    setValue(positionFor(event->position().x()));
    emit sliderMoved(m_value);
    update();
}

void SeekBar::mouseMoveEvent(QMouseEvent *event)
{
    m_hoverX = event->position().x();
    if (m_dragging) {
        setValue(positionFor(event->position().x()));
        emit sliderMoved(m_value);
    }
    update();
}

void SeekBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        emit sliderReleased(m_value);
        update();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void SeekBar::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void SeekBar::leaveEvent(QEvent *event)
{
    m_hovered = false;
    m_hoverX = -1.0;
    update();
    QWidget::leaveEvent(event);
}

// --- IconButton -------------------------------------------------------------

IconButton::IconButton(Icons::Kind kind, const QString &hook, int buttonSize, QWidget *parent)
    : QPushButton(parent)
    , m_kind(kind)
{
    setProperty("dataUi", hook);
    setFixedSize(squareSize(buttonSize));
    m_iconSize = qMax(12, qMin(buttonSize - 16, 22));
    QPushButton::setIconSize(squareSize(m_iconSize));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);

    if (hook == QLatin1String("transportMain"))
        m_colorToken = QStringLiteral("accent-contrast");

    if (ThemeManager *manager = ThemeManager::instance())
        connect(manager, &ThemeManager::changed, this, &IconButton::refreshIcon);

    refreshIcon();
}

void IconButton::setKind(Icons::Kind kind)
{
    m_kind = kind;
    refreshIcon();
}

void IconButton::setIconColorToken(const QString &token)
{
    m_colorToken = token;
    refreshIcon();
}

void IconButton::setIconSize(int size)
{
    m_iconSize = size;
    QPushButton::setIconSize(squareSize(size));
    refreshIcon();
}

void IconButton::refreshIcon()
{
    // У выключенной кнопки фон становится нейтральным (@panel-strong), поэтому
    // «accent-contrast» (обычно белый) на нём пропадал в светлых темах.
    const QString token = isEnabled() ? m_colorToken : QStringLiteral("text-muted");
    const QColor color = ThemeUtil::color(token, ThemeUtil::color(QStringLiteral("text"), Qt::white));

    setIcon(Icons::icon(m_kind, color, m_iconSize));
}

void IconButton::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange)
        refreshIcon();

    QPushButton::changeEvent(event);
}