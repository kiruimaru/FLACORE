#ifndef GLASSWIDGETS_H
#define GLASSWIDGETS_H

#include "Icons.h"

#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

class QPaintEvent;

/**
 * «Стеклянная» панель: полупрозрачная подложка, скругление, тонкая рамка,
 * светлый блик сверху и мягкая тень. Основа карточек настроек и панели плеера.
 */
class GlassPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GlassPanel(QWidget *parent = nullptr, bool strong = false);

    void setStrong(bool strong);
    void setInteractive(bool interactive);   // подсветка рамки по наведению
    void setRadius(int radius);              // -1 = из темы
    void setShadowEnabled(bool enabled);
    void setHook(const QString &dataUi);     // QSS-хук: QWidget[dataUi="card"]

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool m_strong = false;
    bool m_interactive = false;
    bool m_hovered = false;
    bool m_shadow = true;
    int m_radius = -1;
};

/**
 * Обложка трека: скруглённая картинка либо аккуратная заглушка-градиент
 * с нотой. Ничего не грузит с диска сама — получает готовый QImage.
 */
class CoverArt : public QWidget
{
    Q_OBJECT

public:
    explicit CoverArt(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void clearImage();
    bool hasImage() const { return !m_image.isNull(); }

    void setRadius(int radius);          // -1 = из темы
    void setPlaceholderScale(qreal scale);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
    int m_radius = -1;
    qreal m_placeholderScale = 0.42;
};

/**
 * Полоса перемотки: тонкий трек, градиентная заливка акцентом, светящийся
 * индикатор, предпросмотр под курсором. API совместим с QSlider там, где это
 * важно (setValue/isSliderDown/sliderMoved), чтобы логика плеера не менялась.
 */
class SeekBar : public QWidget
{
    Q_OBJECT

public:
    explicit SeekBar(QWidget *parent = nullptr);

    void setRange(int minimum, int maximum);
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int value() const { return m_value; }
    void setValue(int value);
    bool isSliderDown() const { return m_dragging; }

    QSize sizeHint() const override;

signals:
    void sliderMoved(int position);
    void sliderReleased(int position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int positionFor(qreal x) const;
    qreal xForPosition(int position) const;
    QRectF trackRect() const;

    int m_minimum = 0;
    int m_maximum = 0;
    int m_value = 0;
    bool m_dragging = false;
    bool m_hovered = false;
    qreal m_hoverX = -1.0;
};

/**
 * Кнопка с векторной иконкой, которая перекрашивается вместе с темой.
 * Роли: transport (круглая), transportMain (акцентная), windowButton, ghost.
 */
class IconButton : public QPushButton
{
    Q_OBJECT

public:
    explicit IconButton(Icons::Kind kind, const QString &hook = QStringLiteral("transport"),
                        int buttonSize = 40, QWidget *parent = nullptr);

    void setKind(Icons::Kind kind);
    /// Токен цвета иконки: text / text-muted / accent-contrast / danger.
    void setIconColorToken(const QString &token);
    void setIconSize(int size);

protected:
    void changeEvent(QEvent *event) override;

public slots:
    void refreshIcon();

private:
    Icons::Kind m_kind;
    QString m_colorToken = QStringLiteral("text");
    int m_iconSize = 18;
};

#endif // GLASSWIDGETS_H