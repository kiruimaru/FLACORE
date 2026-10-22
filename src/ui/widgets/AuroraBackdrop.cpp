#include "AuroraBackdrop.h"

#include "../theme/ThemeManager.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QWindow>
#include <QtMath>

namespace {

QColor tokenColor(const QHash<QString, QString> &tokens, const QString &key,
                  const QColor &fallback = QColor())
{
    const QColor color(tokens.value(key));
    return color.isValid() ? color : fallback;
}

int tokenInt(const QHash<QString, QString> &tokens, const QString &key, int fallback)
{
    bool ok = false;
    const int value = tokens.value(key).toInt(&ok);
    return ok ? value : fallback;
}

/// Быстрое размытие: уменьшение и обратное растягивание с гладкой интерполяцией.
QImage blurImage(const QImage &source, int radius)
{
    if (source.isNull() || radius <= 0)
        return source;

    const int divisor = qBound(2, radius / 2, 24);
    const QSize small(qMax(1, source.width() / divisor), qMax(1, source.height() / divisor));
    const QImage reduced = source.scaled(small, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return reduced.scaled(source.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace

AuroraBackdrop::AuroraBackdrop(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);

    m_timer.setInterval(40);
    connect(&m_timer, &QTimer::timeout, this, &AuroraBackdrop::tick);

    if (ThemeManager *manager = ThemeManager::instance())
        connect(manager, &ThemeManager::changed, this, &AuroraBackdrop::refresh);

    refresh();
}

void AuroraBackdrop::setCornerRadius(int radius)
{
    if (m_radius == radius)
        return;
    m_radius = qMax(0, radius);
    update();
}

void AuroraBackdrop::setPanelInset(int inset)
{
    m_inset = qMax(0, inset);
    update();
}

void AuroraBackdrop::setFramelessMode(bool frameless)
{
    m_frameless = frameless;
    setAttribute(Qt::WA_TransparentForMouseEvents, !frameless);
    setMouseTracking(frameless);
    setCursor(Qt::ArrowCursor);
    update();
}

Qt::Edges AuroraBackdrop::edgesAt(const QPoint &position) const
{
    const int grip = qMax(6, m_inset + 3);
    Qt::Edges edges;
    if (position.x() <= grip)
        edges |= Qt::LeftEdge;
    if (position.x() >= width() - grip)
        edges |= Qt::RightEdge;
    if (position.y() <= grip)
        edges |= Qt::TopEdge;
    if (position.y() >= height() - grip)
        edges |= Qt::BottomEdge;
    return edges;
}

void AuroraBackdrop::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_frameless) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const Qt::Edges edges = edgesAt(event->position().toPoint());
    if ((edges & Qt::LeftEdge) && (edges & Qt::TopEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if ((edges & Qt::RightEdge) && (edges & Qt::BottomEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if ((edges & Qt::RightEdge) && (edges & Qt::TopEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if ((edges & Qt::LeftEdge) && (edges & Qt::BottomEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if (edges & (Qt::LeftEdge | Qt::RightEdge))
        setCursor(Qt::SizeHorCursor);
    else if (edges & (Qt::TopEdge | Qt::BottomEdge))
        setCursor(Qt::SizeVerCursor);
    else
        setCursor(Qt::ArrowCursor);

    QWidget::mouseMoveEvent(event);
}

void AuroraBackdrop::mousePressEvent(QMouseEvent *event)
{
    if (!m_frameless || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const Qt::Edges edges = edgesAt(event->position().toPoint());
    if (edges == Qt::Edges()) {
        QWidget::mousePressEvent(event);
        return;
    }

    // Размер меняет сама система — гладко и без ручной геометрии.
    if (QWindow *handle = window()->windowHandle()) {
        handle->startSystemResize(edges);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void AuroraBackdrop::leaveEvent(QEvent *event)
{
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

void AuroraBackdrop::refresh()
{
    const ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return;

    const QHash<QString, QString> &tokens = manager->tokens().values;
    m_animations = tokens.value(QStringLiteral("animations"), QStringLiteral("1")) == QLatin1String("1");
    m_blur = tokenInt(tokens, QStringLiteral("bg-blur"), 24);

    const QString path = tokens.value(QStringLiteral("bg-image"));
    if (path != m_backgroundPath) {
        m_backgroundPath = path;
        m_background = QImage();
        m_blurred = QImage();
    }

    if (m_animations && isVisible() && !m_timer.isActive())
        m_timer.start();
    else if (!m_animations && m_timer.isActive())
        m_timer.stop();

    update();
}

void AuroraBackdrop::tick()
{
    // 24 секунды на полный круг — движение почти незаметное, но живое.
    m_phase += 1.0 / (24.0 * 25.0);
    if (m_phase > 1.0)
        m_phase -= 1.0;
    update();
}

void AuroraBackdrop::ensureBackground()
{
    const bool needsLoad = m_background.isNull() && !m_backgroundPath.isEmpty();
    const bool needsBlur = m_backgroundPath.isEmpty()
                           || m_blurredFor != size() || m_blurred.isNull();

    if (needsLoad) {
        QImage loaded;
        if (loaded.load(m_backgroundPath))
            m_background = loaded;
        else
            m_backgroundPath.clear(); // битый файл — работаем без фона
    }

    if (!m_background.isNull() && needsBlur && size().isValid()) {
        const QImage fitted = m_background.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
        const QRect crop((fitted.width() - width()) / 2, (fitted.height() - height()) / 2, width(),
                         height());
        m_blurred = blurImage(fitted.copy(crop), m_blur);
        m_blurredFor = size();
    }
}

void AuroraBackdrop::paintEvent(QPaintEvent *)
{
    const ThemeManager *manager = ThemeManager::instance();
    const QHash<QString, QString> tokens = manager ? manager->tokens().values : QHash<QString, QString>();
    const bool isLight = manager && manager->tokens().isLight;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath clip;
    const QRectF panel = QRectF(rect()).adjusted(m_inset + 0.5, m_inset + 0.5, -m_inset - 0.5,
                                                 -m_inset - 0.5);
    if (m_radius > 0) {
        // Мягкая тень вокруг панели — окно «висит» над рабочим столом.
        const QColor shadow = tokenColor(tokens, QStringLiteral("shadow"), QColor(0, 0, 0, 120));
        painter.setPen(Qt::NoPen);
        for (int i = m_inset; i >= 1; --i) {
            QColor layer = shadow;
            layer.setAlphaF(qBound(0.0, shadow.alphaF() / (i * 1.8), 1.0));
            painter.setBrush(layer);
            painter.drawRoundedRect(panel.adjusted(-i * 0.6, -i * 0.6, i * 0.6, i * 0.6),
                                    m_radius + i * 0.6, m_radius + i * 0.6);
        }
        clip.addRoundedRect(panel, m_radius, m_radius);
        painter.setClipPath(clip);
    }

    const QColor background = tokenColor(tokens, QStringLiteral("bg-primary"), QColor(0x0b, 0x0d, 0x13));
    painter.fillRect(rect(), background);

    ensureBackground();
    if (!m_blurred.isNull()) {
        painter.drawImage(rect(), m_blurred);
        const QColor veil = isLight ? QColor(255, 255, 255, 150) : QColor(0, 0, 0, 140);
        painter.fillRect(rect(), veil);
    }

    // Пятна акцента: три медленно плавающих радиальных градиента.
    const QColor accent = tokenColor(tokens, QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    const double angle = m_phase * 2.0 * M_PI;
    const double strength = isLight ? 0.12 : 0.20;
    const QPointF center(width() / 2.0, height() / 2.0);
    const QList<QPointF> spots = {
        center + QPointF(qCos(angle) * width() * 0.22, qSin(angle) * height() * 0.20),
        center + QPointF(qCos(angle + 2.1) * width() * 0.34, qSin(angle + 2.1) * height() * 0.28),
        center + QPointF(qCos(angle + 4.2) * width() * 0.18, qSin(angle + 4.2) * height() * 0.32),
    };

    int index = 0;
    for (const QPointF &spot : spots) {
        const double radius = qMax(width(), height()) * (index == 1 ? 0.55 : 0.42);
        QRadialGradient gradient(spot, radius);
        QColor start = accent;
        start.setAlphaF(strength * (index == 1 ? 0.85 : 1.0));
        gradient.setColorAt(0.0, start);
        gradient.setColorAt(1.0, Qt::transparent);
        painter.fillRect(rect(), gradient);
        ++index;
    }

    // Вертикальная вуаль: снизу темнее, чтобы панели читались.
    QLinearGradient veilGradient(0, 0, 0, height());
    if (isLight) {
        veilGradient.setColorAt(0.0, QColor(255, 255, 255, 40));
        veilGradient.setColorAt(1.0, QColor(240, 242, 248, 120));
    } else {
        veilGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
        veilGradient.setColorAt(1.0, QColor(0, 0, 0, 90));
    }
    painter.fillRect(rect(), veilGradient);

    if (m_radius > 0) {
        painter.setClipping(false);
        QPen border(tokenColor(tokens, QStringLiteral("border"), QColor(255, 255, 255, 24)));
        border.setWidthF(1.0);
        painter.setPen(border);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(panel, m_radius, m_radius);
    }
}