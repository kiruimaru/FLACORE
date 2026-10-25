#include "AppearancePage.h"

#include "ThemeTile.h"

#include "../theme/ThemeManager.h"
#include "../theme/ThemeUtil.h"
#include "../widgets/GlassWidgets.h"
#include "../widgets/Icons.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QUrl>

namespace {

const char *kFramelessKey = "ui/frameless";

const QStringList kAccentPresets = {
    QStringLiteral("#ff5500"), QStringLiteral("#ff8a3d"), QStringLiteral("#eab308"),
    QStringLiteral("#22c55e"), QStringLiteral("#14b8a6"), QStringLiteral("#38bdf8"),
    QStringLiteral("#7aa2ff"), QStringLiteral("#8b5cf6"), QStringLiteral("#ec4899"),
    QStringLiteral("#f43f5e"),
};

const char *kExampleCss = R"(/* Свой CSS ложится поверх темы и главнее её.
   Вместо CSS-переменных используются токены с @. */

QPushButton[variant="accent"] {
    border-radius: 14px;
}

QWidget[dataUi="player"] {
    border: 1px solid @border-strong;
}

QLabel[role="title"] {
    color: @accent;
}

QWidget[dataUi="sidebar"] {
    background: @panel;
}
)";

/// Токены, доступные в своём CSS (подставляются вместо них).
const char *kTokenHint =
    "Токены для @ (подставляются автоматически):\n"
    "  цвета — accent, accent-hover, accent-glow, accent-soft, accent-contrast,\n"
    "          bg-primary, bg-elevated, panel, panel-strong, panel-solid,\n"
    "          border, border-strong, text, text-muted, text-faint,\n"
    "          hover-bg, sel-bg, shadow, danger, success;\n"
    "  геометрия — radius, radius-sm, radius-lg, font-size, font-size-sm, font-size-lg;\n"
    "  шрифт — font-sans.\n"
    "Хуки (не зависят от вёрстки): QWidget[dataUi=\"titlebar\" | \"sidebar\" | \"main\" |\n"
    "  \"player\" | \"card\" | \"trackList\" | \"results\" | \"search\" | \"cssEditor\" |\n"
    "  \"themeTile\" | \"transport\" | \"transportMain\" | \"windowButton\" | \"toast\"],\n"
    "  QPushButton[variant=\"accent\" | \"ghost\" | \"danger\"].";

QString fontDisplayName(const QString &id)
{
    if (id == QLatin1String("segui"))
        return QStringLiteral("Segoe UI (системный)");
    if (id == QLatin1String("inter"))
        return QStringLiteral("Inter");
    if (id == QLatin1String("manrope"))
        return QStringLiteral("Manrope");
    if (id == QLatin1String("onest"))
        return QStringLiteral("Onest");
    if (id == QLatin1String("golos"))
        return QStringLiteral("Golos Text");
    if (id == QLatin1String("montserrat"))
        return QStringLiteral("Montserrat");
    if (id == QLatin1String("rubik"))
        return QStringLiteral("Rubik");
    if (id == QLatin1String("nunito"))
        return QStringLiteral("Nunito");
    if (id == QLatin1String("raleway"))
        return QStringLiteral("Raleway");
    if (id == QLatin1String("comfortaa"))
        return QStringLiteral("Comfortaa");
    if (id == QLatin1String("system"))
        return QStringLiteral("Системный");
    // Issue #10: пользовательский шрифт — показываем "Свой шрифт".
    if (id == QLatin1String("custom"))
        return QStringLiteral("Свой шрифт…");
    return id;
}

QPixmap swatchPixmap(const QColor &color)
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(1, 1, 20, 20), 6, 6);
    painter.setPen(QPen(QColor(0, 0, 0, 40), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(1, 1, 20, 20), 6, 6);
    return pixmap;
}

/// Карточка с заголовком; возвращает вертикальный слой для содержимого.
QVBoxLayout *addCard(QWidget *owner, QVBoxLayout *pageLayout, const QString &title,
                     const QString &subtitle, QWidget **cardOut = nullptr)
{
    auto *card = new GlassPanel(owner);
    card->setHook(QStringLiteral("card"));

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 16);
    cardLayout->setSpacing(10);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("role", "section");
    cardLayout->addWidget(titleLabel);

    if (!subtitle.isEmpty()) {
        auto *subtitleLabel = new QLabel(subtitle, card);
        subtitleLabel->setProperty("role", "caption");
        subtitleLabel->setWordWrap(true);
        cardLayout->addWidget(subtitleLabel);
    }

    pageLayout->addWidget(card);
    if (cardOut)
        *cardOut = card;
    return cardLayout;
}

QSlider *addSliderRow(QWidget *card, QVBoxLayout *layout, const QString &caption, int minimum,
                      int maximum, int value, QLabel **valueLabelOut = nullptr)
{
    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    auto *label = new QLabel(caption, card);
    label->setProperty("role", "caption");
    label->setMinimumWidth(190);
    row->addWidget(label);

    auto *slider = new QSlider(Qt::Horizontal, card);
    slider->setRange(minimum, maximum);
    slider->setValue(value);
    row->addWidget(slider, 1);

    auto *valueLabel = new QLabel(QString::number(value), card);
    valueLabel->setProperty("role", "faint");
    valueLabel->setMinimumWidth(44);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(valueLabel);

    if (valueLabelOut)
        *valueLabelOut = valueLabel;
    layout->addLayout(row);
    return slider;
}

} // namespace

bool AppearancePage::framelessEnabled()
{
#ifdef Q_OS_WIN
    const bool fallback = true;
#else
    const bool fallback = false;
#endif
    return QSettings().value(QString::fromLatin1(kFramelessKey), fallback).toBool();
}

void AppearancePage::setFramelessEnabled(bool enabled)
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(kFramelessKey), enabled);
    settings.sync();
}

AppearancePage::AppearancePage(QWidget *parent)
    : QWidget(parent)
{
    setProperty("dataUi", "main");

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto *content = new QWidget(scroll);
    content->setProperty("dataUi", "main");
    scroll->setWidget(content);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(18, 16, 18, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("Оформление"), content);
    title->setProperty("role", "title");
    layout->addWidget(title);

    auto *subtitle = new QLabel(tr("Темы как пакеты (theme.json + theme.qss + assets), акцент, фон, "
                                   "шрифт, стекло и свой CSS. Всё применяется сразу и запоминается."),
                                content);
    subtitle->setProperty("role", "caption");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    buildThemeCard(content, layout);
    buildAccentCard(content, layout);
    buildBackgroundCard(content, layout);
    buildTypeCard(content, layout);
    buildGlassCard(content, layout);
    buildCssCard(content, layout);

    m_statusLabel = new QLabel(content);
    m_statusLabel->setProperty("role", "caption");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);
    layout->addStretch(1);

    auto *manager = ThemeManager::instance();
    if (manager) {
        connect(manager, &ThemeManager::themeListChanged, this, &AppearancePage::refreshThemes);
        connect(manager, &ThemeManager::changed, this, &AppearancePage::refreshControls);
        connect(manager, &ThemeManager::problemsChanged, this, [this](const QStringList &problems) {
            if (problems.isEmpty()) {
                m_cssProblems->clear();
                return;
            }
            m_cssProblems->setText(problems.join(QStringLiteral("\n")));
        });
    }

    m_cssTimer = new QTimer(this);
    m_cssTimer->setSingleShot(true);
    m_cssTimer->setInterval(700);
    connect(m_cssTimer, &QTimer::timeout, this, &AppearancePage::applyCustomCssNow);

    refreshThemes();
    refreshControls();
}

// --- карточки ---------------------------------------------------------------

void AppearancePage::buildThemeCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Тема"),
                            tr("Одна тема — одна папка. Своя тема перекрывает встроенную с тем же id."));

    m_themeGrid = new QGridLayout();
    m_themeGrid->setSpacing(10);
    content->addLayout(m_themeGrid);

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(8);

    auto *install = new QPushButton(tr("Установить тему…"), parent);
    install->setProperty("variant", "accent");
    auto *exportButton = new QPushButton(tr("Экспорт текущей…"), parent);
    auto *open = new QPushButton(tr("Открыть папку тем"), parent);
    auto *remove = new QPushButton(tr("Удалить тему"), parent);
    remove->setProperty("variant", "danger");

    buttons->addWidget(install);
    buttons->addWidget(exportButton);
    buttons->addWidget(open);
    buttons->addWidget(remove);
    buttons->addStretch(1);
    content->addLayout(buttons);

    m_themeHint = new QLabel(parent);
    m_themeHint->setProperty("role", "faint");
    m_themeHint->setWordWrap(true);
    m_themeHint->setText(tr("Свои темы лежат в %1. Формат: theme.json (id, name, description, "
                            "version, author, accent, background, font, необязательный tokens) + "
                            "необязательный theme.qss + папка assets/.")
                             .arg(QDir::toNativeSeparators(ThemeManager::userThemesDir())));
    content->addWidget(m_themeHint);

    connect(install, &QPushButton::clicked, this, &AppearancePage::installThemeFromDisk);
    connect(exportButton, &QPushButton::clicked, this, &AppearancePage::exportCurrentTheme);
    connect(open, &QPushButton::clicked, this, []() {
        ThemeManager::instance()->openThemesFolder();
    });
    connect(remove, &QPushButton::clicked, this, &AppearancePage::removeSelectedTheme);
}

void AppearancePage::buildAccentCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Акцент"),
                            tr("Акцент задаёт заливку кнопок, выделение и свечение фона."));

    auto *row = new QHBoxLayout();
    row->setSpacing(6);

    for (const QString &preset : kAccentPresets) {
        auto *button = new QPushButton(parent);
        button->setProperty("variant", "ghost");
        button->setFixedSize(34, 34);
        button->setIconSize(QSize(22, 22));
        button->setIcon(swatchPixmap(QColor(preset)));
        button->setToolTip(preset);
        row->addWidget(button);
        connect(button, &QPushButton::clicked, this, [preset]() {
            ThemeManager::instance()->setAccentOverride(preset);
        });
    }

    auto *custom = new QPushButton(tr("Свой цвет…"), parent);
    row->addSpacing(6);
    row->addWidget(custom);
    row->addStretch(1);
    content->addLayout(row);

    m_accentLabel = new QLabel(parent);
    m_accentLabel->setProperty("role", "caption");
    content->addWidget(m_accentLabel);

    auto *bottom = new QHBoxLayout();
    bottom->setSpacing(8);

    m_accentFromCover = new QCheckBox(tr("Брать акцент из обложки трека"), parent);
    m_accentFromCover->setToolTip(tr("Цвет обложки становится акцентом интерфейса"));
    bottom->addWidget(m_accentFromCover);

    auto *reset = new QPushButton(tr("Сбросить акцент"), parent);
    bottom->addWidget(reset);
    bottom->addStretch(1);
    content->addLayout(bottom);

    connect(custom, &QPushButton::clicked, this, [this]() {
        const QColor current(ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00)));
        const QColor picked = QColorDialog::getColor(current, this, tr("Акцентный цвет"));
        if (picked.isValid())
            ThemeManager::instance()->setAccentOverride(picked.name(QColor::HexRgb));
    });
    connect(m_accentFromCover, &QCheckBox::toggled, this, [](bool checked) {
        ThemeManager::instance()->setAccentFromCover(checked);
    });
    connect(reset, &QPushButton::clicked, this, []() {
        ThemeManager::instance()->setAccentOverride(QString());
    });
}

void AppearancePage::buildBackgroundCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Фон окна"),
                            tr("Картинка под стеклом панелей. Размытие зависит от режима производительности."));

    auto *row = new QHBoxLayout();
    row->setSpacing(8);

    m_backgroundPath = new QLineEdit(parent);
    m_backgroundPath->setReadOnly(true);
    m_backgroundPath->setPlaceholderText(tr("фон не задан — используется градиент темы"));
    row->addWidget(m_backgroundPath, 1);

    auto *file = new QPushButton(tr("Файл…"), parent);
    auto *url = new QPushButton(tr("По ссылке…"), parent);
    auto *clear = new QPushButton(tr("Убрать"), parent);
    clear->setProperty("variant", "danger");

    row->addWidget(file);
    row->addWidget(url);
    row->addWidget(clear);
    content->addLayout(row);

    QLabel *blurLabel = nullptr;
    m_backgroundBlur = addSliderRow(parent, content, tr("Размытие фона"), 0, 64, 32, &blurLabel);

    connect(file, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("Фоновое изображение"), QString(),
                                                         tr("Изображения (*.png *.jpg *.jpeg *.webp "
                                                            "*.bmp);;Все файлы (*)"));
        if (!path.isEmpty())
            ThemeManager::instance()->setBackgroundImage(path);
    });
    connect(url, &QPushButton::clicked, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Ссылка на изображение"));
        auto *dialogLayout = new QVBoxLayout(&dialog);
        auto *edit = new QLineEdit(&dialog);
        edit->setPlaceholderText(QStringLiteral("https://…/wallpaper.jpg"));
        dialogLayout->addWidget(edit);
        auto *buttonRow = new QHBoxLayout();
        auto *ok = new QPushButton(tr("Скачать"), &dialog);
        ok->setProperty("variant", "accent");
        auto *cancel = new QPushButton(tr("Отмена"), &dialog);
        buttonRow->addStretch(1);
        buttonRow->addWidget(cancel);
        buttonRow->addWidget(ok);
        dialogLayout->addLayout(buttonRow);
        connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
        connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() == QDialog::Accepted && !edit->text().trimmed().isEmpty())
            downloadBackground(edit->text().trimmed());
    });
    connect(clear, &QPushButton::clicked, this, []() {
        ThemeManager::instance()->setBackgroundImage(QString());
    });
    connect(m_backgroundBlur, &QSlider::valueChanged, this, [this, blurLabel](int value) {
        blurLabel->setText(QString::number(value));
        ThemeManager::instance()->setBackgroundBlur(value);
    });
}

void AppearancePage::buildTypeCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Шрифт"),
                            tr("Семейства из каталога тем; если шрифта нет в системе, берётся системный."));

    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    auto *label = new QLabel(tr("Семейство"), parent);
    label->setProperty("role", "caption");
    label->setMinimumWidth(190);
    row->addWidget(label);

    m_fontCombo = new QComboBox(parent);
    for (const QString &id : availableFontIds())
        m_fontCombo->addItem(fontDisplayName(id), id);
    row->addWidget(m_fontCombo, 1);
    content->addLayout(row);

    // Issue #10: поле ввода для пользовательского шрифта ("Свой шрифт…").
    // Видно только когда выбрано "custom".
    auto *customRow = new QHBoxLayout();
    customRow->setSpacing(10);
    auto *customLabel = new QLabel(tr("Имя семейства"), parent);
    customLabel->setProperty("role", "caption");
    customLabel->setMinimumWidth(190);
    customRow->addWidget(customLabel);
    m_customFontEdit = new QLineEdit(parent);
    m_customFontEdit->setPlaceholderText(tr("Например: JetBrains Mono, Ubuntu, Roboto…"));
    const QString savedCustomFont = QSettings().value(
        QStringLiteral("appearance/customFontFamily")).toString();
    m_customFontEdit->setText(savedCustomFont);
    customRow->addWidget(m_customFontEdit, 1);
    content->addLayout(customRow);

    // Показываем/скрываем поле ввода в зависимости от выбранного шрифта.
    const bool isCustom = m_fontCombo->currentData().toString() == QLatin1String("custom");
    customLabel->setVisible(isCustom);
    m_customFontEdit->setVisible(isCustom);

    QLabel *scaleLabel = nullptr;
    m_fontScale = addSliderRow(parent, content, tr("Масштаб текста, %"), 80, 150, 100, &scaleLabel);

    connect(m_fontCombo, &QComboBox::currentIndexChanged, this, [this, customLabel](int index) {
        if (index < 0)
            return;
        const QString id = m_fontCombo->itemData(index).toString();
        const bool custom = (id == QLatin1String("custom"));
        customLabel->setVisible(custom);
        m_customFontEdit->setVisible(custom);
        ThemeManager::instance()->setFontOverride(id);
    });
    connect(m_customFontEdit, &QLineEdit::textChanged, this, [](const QString &text) {
        // Сохраняем имя шрифта и перезагружаем тему.
        QSettings().setValue(QStringLiteral("appearance/customFontFamily"), text.trimmed());
        if (ThemeManager *mgr = ThemeManager::instance())
            mgr->setFontOverride(QStringLiteral("custom"));
    });
    connect(m_fontScale, &QSlider::valueChanged, this, [this, scaleLabel](int value) {
        scaleLabel->setText(QStringLiteral("%1%").arg(value));
        ThemeManager::instance()->setFontScalePercent(value);
    });
}

void AppearancePage::buildGlassCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Стекло и производительность"),
                            tr("Режим «Лайт» выключает размытие и анимации — для слабых машин."));

    auto *modeRow = new QHBoxLayout();
    modeRow->setSpacing(10);
    auto *modeLabel = new QLabel(tr("Режим"), parent);
    modeLabel->setProperty("role", "caption");
    modeLabel->setMinimumWidth(190);
    modeRow->addWidget(modeLabel);
    m_modeCombo = new QComboBox(parent);
    m_modeCombo->addItem(tr("Красота"), QStringLiteral("beauty"));
    m_modeCombo->addItem(tr("Средний"), QStringLiteral("medium"));
    m_modeCombo->addItem(tr("Лайт"), QStringLiteral("light"));
    modeRow->addWidget(m_modeCombo, 1);
    content->addLayout(modeRow);

    QLabel *radiusLabel = nullptr;
    QLabel *glassLabel = nullptr;
    QLabel *blurLabel = nullptr;
    m_radiusSlider = addSliderRow(parent, content, tr("Скругление, px"), 0, 32, 16, &radiusLabel);
    m_glassSlider = addSliderRow(parent, content, tr("Плотность стекла, %"), 20, 100, 55, &glassLabel);
    m_blurSlider = addSliderRow(parent, content, tr("Размытие стекла, px"), 0, 48, 24, &blurLabel);

    auto *bottom = new QHBoxLayout();
    bottom->setSpacing(10);
    m_animations = new QCheckBox(tr("Анимации фона"), parent);
    m_frameless = new QCheckBox(tr("Своя шапка окна (безрамочный режим)"), parent);
    auto *resetAll = new QPushButton(tr("Сбросить оформление"), parent);
    resetAll->setProperty("variant", "danger");
    bottom->addWidget(m_animations);
    bottom->addWidget(m_frameless);
    bottom->addStretch(1);
    bottom->addWidget(resetAll);
    content->addLayout(bottom);

    connect(m_modeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            ThemeManager::instance()->setMode(m_modeCombo->itemData(index).toString());
    });
    connect(m_radiusSlider, &QSlider::valueChanged, this, [this, radiusLabel](int value) {
        radiusLabel->setText(QString::number(value));
        ThemeManager::instance()->setRadiusOverride(value);
    });
    connect(m_glassSlider, &QSlider::valueChanged, this, [this, glassLabel](int value) {
        glassLabel->setText(QStringLiteral("%1%").arg(value));
        ThemeManager::instance()->setGlassAlphaPercent(value);
    });
    connect(m_blurSlider, &QSlider::valueChanged, this, [this, blurLabel](int value) {
        blurLabel->setText(QString::number(value));
        ThemeManager::instance()->setBlurOverride(value);
    });
    connect(m_animations, &QCheckBox::toggled, this, [](bool checked) {
        ThemeManager::instance()->setAnimationsOverride(checked ? 1 : 0);
    });
    connect(m_frameless, &QCheckBox::toggled, this, [this](bool checked) {
        setFramelessEnabled(checked);
        emit framelessToggled(checked);
    });
    connect(resetAll, &QPushButton::clicked, this, [this]() {
        ThemeManager::instance()->resetAppearance();
        setFramelessEnabled(framelessEnabled());
        refreshControls();
        m_statusLabel->setText(tr("Оформление сброшено к теме по умолчанию."));
    });
}

void AppearancePage::buildCssCard(QWidget *parent, QVBoxLayout *layout)
{
    auto *content = addCard(parent, layout, tr("Свой CSS"),
                            tr("Ложится поверх темы и главнее её. Применяется через 0,7 секунды после "
                               "правки; выключатель в заголовке сохраняет текст."));

    m_cssEnabled = new QCheckBox(tr("Свой CSS включён"), parent);
    content->addWidget(m_cssEnabled);

    m_cssEditor = new QPlainTextEdit(parent);
    m_cssEditor->setProperty("dataUi", "cssEditor");
    m_cssEditor->setPlaceholderText(kExampleCss);
    m_cssEditor->setMinimumHeight(190);
    content->addWidget(m_cssEditor);

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    auto *apply = new QPushButton(tr("Применить"), parent);
    apply->setProperty("variant", "accent");
    auto *example = new QPushButton(tr("Вставить пример"), parent);
    auto *clear = new QPushButton(tr("Очистить"), parent);
    clear->setProperty("variant", "danger");
    buttons->addWidget(apply);
    buttons->addWidget(example);
    buttons->addWidget(clear);
    buttons->addStretch(1);
    content->addLayout(buttons);

    m_cssProblems = new QLabel(parent);
    m_cssProblems->setProperty("role", "danger");
    m_cssProblems->setWordWrap(true);
    content->addWidget(m_cssProblems);

    auto *hint = new QLabel(QString::fromUtf8(kTokenHint), parent);
    hint->setProperty("role", "faint");
    hint->setWordWrap(true);
    content->addWidget(hint);

    connect(m_cssEditor, &QPlainTextEdit::textChanged, this, &AppearancePage::scheduleCustomCssApply);
    connect(apply, &QPushButton::clicked, this, &AppearancePage::applyCustomCssNow);
    connect(example, &QPushButton::clicked, this, [this]() {
        m_cssEditor->setPlainText(QString::fromUtf8(kExampleCss));
    });
    connect(clear, &QPushButton::clicked, this, [this]() {
        m_cssEditor->clear();
        ThemeManager::instance()->setCustomCss(QString());
        m_statusLabel->setText(tr("Свой CSS очищен."));
    });
    connect(m_cssEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        ThemeManager::instance()->setCustomCssEnabled(checked);
        m_statusLabel->setText(checked ? tr("Свой CSS включён.") : tr("Свой CSS выключен."));
    });
}

// --- обновление состояния ---------------------------------------------------

void AppearancePage::refreshThemes()
{
    ThemeManager *manager = ThemeManager::instance();
    if (!manager || !m_themeGrid)
        return;

    for (ThemeTile *tile : m_tiles) {
        m_themeGrid->removeWidget(tile);
        tile->deleteLater();
    }
    m_tiles.clear();

    const QList<Theme> themes = manager->themes();
    const QString activeId = manager->currentThemeId();

    int row = 0;
    int column = 0;
    for (const Theme &theme : themes) {
        auto *tile = new ThemeTile(theme, this);
        tile->setActive(theme.id == activeId);
        m_themeGrid->addWidget(tile, row, column);
        m_tiles.append(tile);

        connect(tile, &QPushButton::clicked, this, [this, theme]() {
            ThemeManager::instance()->setCurrentTheme(theme.id);
            for (ThemeTile *item : m_tiles)
                item->setActive(item->theme().id == theme.id);
            m_statusLabel->setText(tr("Тема «%1» применена.").arg(theme.name));
        });

        if (++column >= 3) {
            column = 0;
            ++row;
        }
    }

    // Пустые места не растягиваются: карточки одинаковой ширины.
    for (int i = 0; i < 3; ++i)
        m_themeGrid->setColumnStretch(i, 1);
}

void AppearancePage::refreshControls()
{
    ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return;

    const QHash<QString, QString> &tokens = manager->tokens().values;

    const QSignalBlocker blockAccent(m_accentFromCover);
    m_accentFromCover->setChecked(manager->accentFromCover());
    const QColor accent = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    m_accentLabel->setText(tr("Текущий акцент: %1").arg(accent.name(QColor::HexRgb)));

    const QString backgroundImage = manager->backgroundImage();
    m_backgroundPath->setText(backgroundImage.isEmpty()
                                  ? QString()
                                  : QDir::toNativeSeparators(backgroundImage));
    {
        const QSignalBlocker block(m_backgroundBlur);
        m_backgroundBlur->setValue(tokens.value(QStringLiteral("bg-blur"), QStringLiteral("32")).toInt());
    }

    {
        const QSignalBlocker block(m_fontCombo);
        QString fontId = manager->fontOverride();
        if (fontId.isEmpty())
            fontId = manager->currentTheme().font;
        const int index = m_fontCombo->findData(fontId);
        m_fontCombo->setCurrentIndex(index >= 0 ? index : 0);

        // Issue #10: обновляем видимость поля пользовательского шрифта.
        const bool isCustom = (m_fontCombo->currentData().toString() == QLatin1String("custom"));
        if (m_customFontEdit) {
            m_customFontEdit->setVisible(isCustom);
            // Обновляем поле, если значение изменилось снаружи.
            const QString savedCustomFont = QSettings().value(
                QStringLiteral("appearance/customFontFamily")).toString();
            if (m_customFontEdit->text() != savedCustomFont) {
                const QSignalBlocker blockEdit(m_customFontEdit);
                m_customFontEdit->setText(savedCustomFont);
            }
        }
    }
    {
        const QSignalBlocker block(m_fontScale);
        m_fontScale->setValue(manager->fontScalePercent());
    }
    {
        const QSignalBlocker block(m_modeCombo);
        const int index = m_modeCombo->findData(manager->mode());
        m_modeCombo->setCurrentIndex(index >= 0 ? index : 0);
    }
    {
        const QSignalBlocker block(m_radiusSlider);
        m_radiusSlider->setValue(tokens.value(QStringLiteral("radius"), QStringLiteral("16")).toInt());
    }
    {
        const QSignalBlocker block(m_glassSlider);
        const double alpha = tokens.value(QStringLiteral("glass-alpha"), QStringLiteral("0.55")).toDouble();
        m_glassSlider->setValue(int(qRound(alpha * 100)));
    }
    {
        const QSignalBlocker block(m_blurSlider);
        m_blurSlider->setValue(tokens.value(QStringLiteral("blur"), QStringLiteral("24")).toInt());
    }
    {
        const QSignalBlocker block(m_animations);
        m_animations->setChecked(tokens.value(QStringLiteral("animations"), QStringLiteral("1"))
                                 == QLatin1String("1"));
    }
    {
        const QSignalBlocker block(m_frameless);
        m_frameless->setChecked(framelessEnabled());
    }
    {
        const QSignalBlocker block(m_cssEnabled);
        m_cssEnabled->setChecked(manager->customCssEnabled());
    }
    if (m_cssEditor->toPlainText() != manager->customCss()) {
        const QSignalBlocker block(m_cssEditor);
        m_cssEditor->setPlainText(manager->customCss());
    }

    for (ThemeTile *tile : m_tiles)
        tile->setActive(tile->theme().id == manager->currentThemeId());
}

void AppearancePage::scheduleCustomCssApply()
{
    m_cssTimer->start();
}

void AppearancePage::applyCustomCssNow()
{
    ThemeManager *manager = ThemeManager::instance();
    if (!manager)
        return;

    const QString css = m_cssEditor->toPlainText();
    if (css == manager->customCss())
        return;

    manager->setCustomCss(css);
    m_statusLabel->setText(tr("Свой CSS применён (%1 символов).").arg(css.size()));
}

void AppearancePage::downloadBackground(const QString &url)
{
    auto *manager = new QNetworkAccessManager(this);
    m_statusLabel->setText(tr("Скачиваю фон…"));

    connect(manager, &QNetworkAccessManager::finished, this,
            [this, manager](QNetworkReply *reply) {
                reply->deleteLater();
                manager->deleteLater();

                if (reply->error() != QNetworkReply::NoError) {
                    m_statusLabel->setText(tr("Не удалось скачать фон: %1").arg(reply->errorString()));
                    return;
                }

                const QByteArray data = reply->readAll();
                QImage image;
                if (!image.loadFromData(data) || image.isNull()) {
                    m_statusLabel->setText(tr("Ссылка вернула не изображение."));
                    return;
                }

                const QString dir = QDir(QStandardPaths::writableLocation(
                                             QStandardPaths::AppDataLocation))
                                        .filePath(QStringLiteral("backgrounds"));
                QDir().mkpath(dir);
                const QString name = reply->url().fileName().isEmpty()
                                         ? QStringLiteral("wallpaper.jpg")
                                         : reply->url().fileName();
                const QString path = QDir(dir).filePath(name);
                if (!image.save(path)) {
                    m_statusLabel->setText(tr("Не удалось сохранить фон в %1").arg(path));
                    return;
                }

                ThemeManager::instance()->setBackgroundImage(path);
                m_statusLabel->setText(tr("Фон применён: %1").arg(QDir::toNativeSeparators(path)));
            });

    manager->get(QNetworkRequest(QUrl(url)));
}

void AppearancePage::installThemeFromDisk()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Установить тему"), QString(),
        tr("Тема FLACORE (theme.json);;Все файлы (*)"));
    if (path.isEmpty())
        return;

    QString error;
    if (ThemeManager::instance()->installTheme(path, &error)) {
        m_statusLabel->setText(tr("Тема установлена."));
        refreshThemes();
    } else {
        QMessageBox::warning(this, tr("Тема не установлена"), error);
    }
}

void AppearancePage::exportCurrentTheme()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Куда выгрузить тему"));
    if (dir.isEmpty())
        return;

    QString error;
    if (ThemeManager::instance()->exportCurrentTheme(dir, &error))
        m_statusLabel->setText(tr("Тема выгружена в %1").arg(QDir::toNativeSeparators(dir)));
    else
        QMessageBox::warning(this, tr("Экспорт не удался"), error);
}

void AppearancePage::removeSelectedTheme()
{
    ThemeManager *manager = ThemeManager::instance();
    const Theme current = manager->currentTheme();
    if (current.builtin) {
        m_statusLabel->setText(tr("Тема «%1» встроенная — её нельзя удалить. "
                                  "Скопируйте её через «Экспорт текущей» и правьте копию.")
                                   .arg(current.name));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Удалить тему"),
        tr("Удалить пользовательскую тему «%1» из %2?").arg(current.name, current.dirPath));
    if (answer != QMessageBox::Yes)
        return;

    QString error;
    if (manager->removeUserTheme(current.id, &error)) {
        m_statusLabel->setText(tr("Тема удалена."));
        refreshThemes();
    } else {
        QMessageBox::warning(this, tr("Не удалось удалить"), error);
    }
}