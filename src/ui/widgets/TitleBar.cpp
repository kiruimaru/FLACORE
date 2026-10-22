#include "TitleBar.h"

#include "GlassWidgets.h"
#include "Icons.h"

#include "../theme/ThemeUtil.h"

#include <QApplication>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setProperty("dataUi", "titlebar");
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(ThemeUtil::number(QStringLiteral("titlebar-height"), 44));
    setAttribute(Qt::WA_Hover, true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 6, 10, 6);
    layout->setSpacing(10);

    auto *mark = new QLabel(this);
    mark->setPixmap(Icons::pixmap(Icons::Kind::Music,
                                 ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00)),
                                 18));
    layout->addWidget(mark);

    m_brand = new QLabel(QStringLiteral("FLACORE"), this);
    m_brand->setProperty("role", "brand");
    layout->addWidget(m_brand);

    m_version = new QLabel(this);
    m_version->setProperty("role", "faint");
    const QString version = QCoreApplication::applicationVersion();
    m_version->setText(version.isEmpty() ? QString() : QStringLiteral("v%1").arg(version));
    m_version->setVisible(!version.isEmpty());
    layout->addWidget(m_version);

    layout->addStretch(1);

    m_search = new QLineEdit(this);
    m_search->setProperty("dataUi", "search");
    m_search->setClearButtonEnabled(true);
    m_search->setFixedWidth(260);
    m_search->setPlaceholderText(tr("Поиск в библиотеке…"));
    layout->addWidget(m_search);

    m_appearance = new IconButton(Icons::Kind::Palette, QStringLiteral("ghost"), 34, this);
    m_appearance->setToolTip(tr("Оформление: темы, акцент, свой CSS"));
    layout->addWidget(m_appearance);

    m_menu = new IconButton(Icons::Kind::Menu, QStringLiteral("ghost"), 34, this);
    m_menu->setToolTip(tr("Меню"));
    layout->addWidget(m_menu);

    m_minimize = new IconButton(Icons::Kind::Minimize, QStringLiteral("windowButton"), 32, this);
    m_minimize->setToolTip(tr("Свернуть"));
    layout->addWidget(m_minimize);

    m_maximize = new IconButton(Icons::Kind::Maximize, QStringLiteral("windowButton"), 32, this);
    m_maximize->setToolTip(tr("Развернуть"));
    layout->addWidget(m_maximize);

    m_close = new IconButton(Icons::Kind::Close, QStringLiteral("windowButton"), 32, this);
    m_close->setProperty("danger", true);
    m_close->setToolTip(tr("Закрыть"));
    layout->addWidget(m_close);

    connect(m_search, &QLineEdit::textChanged, this, &TitleBar::searchChanged);
    connect(m_appearance, &QPushButton::clicked, this, &TitleBar::appearanceRequested);
    connect(m_menu, &QPushButton::clicked, this, [this]() {
        emit menuRequested(m_menu->mapToGlobal(QPoint(0, m_menu->height() + 4)));
    });
    connect(m_minimize, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximize, &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    connect(m_close, &QPushButton::clicked, this, &TitleBar::closeRequested);

    setFrameless(false);
}

void TitleBar::setFrameless(bool frameless)
{
    m_frameless = frameless;
    m_minimize->setVisible(frameless);
    m_maximize->setVisible(frameless);
    m_close->setVisible(frameless);
}

void TitleBar::setMaximized(bool maximized)
{
    m_maximize->setKind(maximized ? Icons::Kind::Restore : Icons::Kind::Maximize);
    m_maximize->setToolTip(maximized ? tr("Восстановить") : tr("Развернуть"));
}

void TitleBar::setSearchPlaceholder(const QString &text)
{
    m_search->setPlaceholderText(text);
}

QString TitleBar::searchText() const
{
    return m_search->text();
}

void TitleBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QColor border = ThemeUtil::color(QStringLiteral("border"), QColor(255, 255, 255, 20));
    painter.setPen(QPen(border, 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (!m_frameless || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    // Системное перетаскивание окна: работает на Windows/Linux/macOS без ручной геометрии.
    if (QWindow *handle = window()->windowHandle()) {
        handle->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_frameless && event->button() == Qt::LeftButton) {
        emit maximizeRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}