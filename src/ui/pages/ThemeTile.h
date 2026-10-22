#ifndef THEMETILE_H
#define THEMETILE_H

#include <QPushButton>

#include "ui/theme/Theme.h"

/**
 * Карточка темы в списке оформления: полоски-образцы цветов, название,
 * автор и описание. Рисуется сама, фон и рамку берёт из QSS (dataUi="themeTile").
 */
class ThemeTile : public QPushButton
{
    Q_OBJECT

public:
    explicit ThemeTile(const Theme &theme, QWidget *parent = nullptr);

    Theme theme() const { return m_theme; }
    void setActive(bool active);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    Theme m_theme;
    bool m_active = false;
};

#endif // THEMETILE_H