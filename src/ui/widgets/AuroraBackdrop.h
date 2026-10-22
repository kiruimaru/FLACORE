#ifndef AURORABACKDROP_H
#define AURORABACKDROP_H

#include <QImage>
#include <QTimer>
#include <QWidget>

/**
 * Подложка окна: базовый цвет темы, необязательное фоновое изображение
 * (с размытием по режиму производительности) и медленно плывущие пятна акцента —
 * тот же приём, что в «liquid glass»-темах эталонного клиента, но на QPainter.
 */
class AuroraBackdrop : public QWidget
{
    Q_OBJECT

public:
    explicit AuroraBackdrop(QWidget *parent = nullptr);

    /// Радиус скругления окна (0 — прямые углы, безрамочный режим).
    void setCornerRadius(int radius);
    int cornerRadius() const { return m_radius; }

    /// Отступ «панели» от края окна: в нём рисуется тень и живут ручки ресайза.
    void setPanelInset(int inset);
    int panelInset() const { return m_inset; }

    /// Безрамочный режим: края окна становятся ручками изменения размера.
    void setFramelessMode(bool frameless);

    /// Перечитать цвета/режим из текущей темы.
    void refresh();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void tick();
    void ensureBackground();
    Qt::Edges edgesAt(const QPoint &position) const;

    QTimer m_timer;
    double m_phase = 0.0;
    int m_radius = 0;
    int m_inset = 0;
    bool m_frameless = false;
    bool m_animations = true;
    QString m_backgroundPath;
    QImage m_background;
    QImage m_blurred;
    QSize m_blurredFor;
    int m_blur = 24;
};

#endif // AURORABACKDROP_H