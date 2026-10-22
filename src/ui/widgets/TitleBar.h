#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

class QLabel;
class QLineEdit;

class IconButton;

/**
 * Шапка окна: название, поиск по библиотеке, быстрый доступ к оформлению,
 * меню и кнопки окна (в безрамочном режиме). Тянется системным перетаскиванием
 * (QWindow::startSystemMove), поэтому окно двигается нативно.
 */
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setFrameless(bool frameless);
    void setMaximized(bool maximized);
    void setSearchPlaceholder(const QString &text);
    QString searchText() const;

signals:
    void searchChanged(const QString &text);
    void appearanceRequested();
    void menuRequested(const QPoint &globalPos);
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *m_brand = nullptr;
    QLabel *m_version = nullptr;
    QLineEdit *m_search = nullptr;
    IconButton *m_appearance = nullptr;
    IconButton *m_menu = nullptr;
    IconButton *m_minimize = nullptr;
    IconButton *m_maximize = nullptr;
    IconButton *m_close = nullptr;
    bool m_frameless = false;
};

#endif // TITLEBAR_H