#include "StyleSheet.h"

namespace StyleSheet {

QString baseTemplate()
{
    // Токены подставляются как @имя (см. Theme.h). Ничего специфичного для темы
    // здесь быть не должно: всё, что не совпадает со вкусом пользователя,
    // перекрывается в theme.qss или в «своём CSS».
    static const char *kTemplate = R"QSS(
/* --- фон и базовые тексты ------------------------------------------------ */
QWidget {
    color: @text;
    font-family: "@font-sans";
    font-size: @font-size;
}
QWidget[dataUi="app"], #auroraBackdrop {
    background: @bg-primary;
}
QWidget[dataUi="main"], QStackedWidget, QScrollArea, QScrollArea > QWidget > QWidget {
    background: transparent;
}
QLabel {
    background: transparent;
}
QLabel[role="title"] {
    font-size: @font-size-title;
    font-weight: 700;
}
QLabel[role="section"] {
    font-size: @font-size-lg;
    font-weight: 600;
}
QLabel[role="caption"] {
    font-size: @font-size-sm;
    color: @text-muted;
}
QLabel[role="muted"] {
    color: @text-muted;
}
QLabel[role="faint"] {
    font-size: @font-size-sm;
    color: @text-faint;
}
QLabel[role="accent"] {
    color: @accent;
    font-weight: 600;
}
QLabel[role="danger"] {
    color: @danger;
}
QLabel[dataUi="badge"] {
    background: @accent-soft;
    color: @accent;
    border-radius: @radius-sm;
    padding: 2px 8px;
    font-size: @font-size-sm;
    font-weight: 600;
}
QLabel[dataUi="toast"] {
    background: @bg-elevated;
    border: 1px solid @border-strong;
    border-radius: @radius-sm;
    padding: 7px 14px;
    color: @text;
}
QLabel[dataUi="toast"][role="danger"] {
    color: @danger;
    border-color: @danger;
}

/* --- окно и его каркас --------------------------------------------------- */
QMainWindow, QDialog {
    background: @bg-primary;
}
QWidget[dataUi="sheet"] {
    background: @bg-elevated;
    border: 1px solid @border;
    border-radius: @radius-lg;
}
QWidget[dataUi="titlebar"] {
    background: transparent;
}
QWidget[dataUi="titlebar"] QLabel[role="brand"] {
    font-size: @font-size-lg;
    font-weight: 800;
    letter-spacing: 1px;
}
QWidget[dataUi="sidebar"] {
    background: @panel;
    border-right: 1px solid @border;
}
QWidget[dataUi="divider"] {
    background: @border;
}
QWidget[dataUi="section-header"] {
    background: @panel;
    border: 1px solid @border;
    border-radius: @radius;
}

/* --- навигация в сайдбаре ------------------------------------------------ */
QListWidget[dataUi="sidebar"] {
    background: transparent;
    border: none;
    outline: none;
    padding: 6px;
}
QListWidget[dataUi="sidebar"]::item {
    color: @text-muted;
    border-radius: @radius-sm;
    padding: 9px 12px;
    margin: 2px 4px;
}
QListWidget[dataUi="sidebar"]::item:hover {
    background: @hover-bg;
    color: @text;
}
QListWidget[dataUi="sidebar"]::item:selected {
    background: @sel-bg;
    color: @text;
    font-weight: 600;
}

/* --- поиск в шапке ------------------------------------------------------- */
QLineEdit[dataUi="search"] {
    background: @panel-strong;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 6px 12px;
    selection-background-color: @accent;
    selection-color: @accent-contrast;
}
QLineEdit[dataUi="search"]:focus {
    border: 1px solid @border-strong;
    background: @panel;
}

/* --- кнопки -------------------------------------------------------------- */
QPushButton {
    background: @panel-strong;
    color: @text;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 7px 14px;
    font-weight: 500;
}
QPushButton:hover {
    background: @hover-bg;
    border-color: @border-strong;
}
QPushButton:pressed {
    background: @sel-bg;
}
QPushButton:disabled {
    color: @text-muted;
    background: @panel-solid;
    border-color: @border;
}
QPushButton:focus {
    outline: none;
}
QPushButton[variant="accent"] {
    background: @accent;
    color: @accent-contrast;
    border: 1px solid @accent;
    font-weight: 600;
}
QPushButton[variant="accent"]:hover {
    background: @accent-hover;
    border-color: @accent-hover;
}
QPushButton[variant="accent"]:disabled {
    background: @panel-strong;
    color: @text-faint;
    border-color: @border;
}
QPushButton[variant="ghost"] {
    background: transparent;
    border: 1px solid transparent;
    color: @text-muted;
}
QPushButton[variant="ghost"]:hover {
    background: @hover-bg;
    color: @text;
    border-color: @border;
}
QPushButton[variant="danger"] {
    background: @danger-soft;
    border: 1px solid @danger-border;
    color: @danger;
}
QPushButton[variant="danger"]:hover {
    background: @danger-border;
    border-color: @danger;
    color: @text;
}
QPushButton[variant="danger"]:disabled {
    background: @panel-solid;
    border-color: @border;
    color: @text-muted;
}
QPushButton[dataUi="windowButton"] {
    background: transparent;
    border: none;
    border-radius: @radius-sm;
    padding: 0px;
}
QPushButton[dataUi="windowButton"]:hover {
    background: @hover-bg;
}
QPushButton[dataUi="windowButton"][danger="true"]:hover {
    background: #ff5f56;
}
QPushButton[dataUi="transport"] {
    background: transparent;
    border: none;
    border-radius: 40px;
    padding: 0px;
}
QPushButton[dataUi="transport"]:hover {
    background: @hover-bg;
}
QPushButton[dataUi="transport"]:disabled {
    background: transparent;
}
QPushButton[dataUi="transportMain"] {
    background: @accent;
    border: 1px solid @accent;
    border-radius: 40px;
    padding: 0px;
}
QPushButton[dataUi="transportMain"]:hover {
    background: @accent-hover;
    border-color: @accent-hover;
}
QPushButton[dataUi="transportMain"]:disabled {
    background: @panel-strong;
    border-color: @border;
}
QPushButton[dataUi="themeTile"] {
    background: @panel;
    border: 1px solid @border;
    border-radius: @radius;
    padding: 10px;
    text-align: left;
}
QPushButton[dataUi="themeTile"]:hover {
    border-color: @border-strong;
    background: @panel-strong;
}
QPushButton[dataUi="themeTile"][active="true"] {
    border: 1px solid @accent;
    background: @sel-bg;
}

/* --- поля ввода, комбобоксы, редакторы ---------------------------------- */
QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox {
    background: @panel-strong;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 6px 10px;
    selection-background-color: @accent;
    selection-color: @accent-contrast;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QSpinBox:focus {
    border: 1px solid @border-strong;
}
QLineEdit:disabled, QPlainTextEdit:disabled {
    color: @text-faint;
}
QPlainTextEdit[dataUi="cssEditor"] {
    font-family: "Consolas", "Cascadia Mono", "DejaVu Sans Mono", monospace;
    font-size: @font-size-sm;
    background: @panel;
}
QComboBox {
    background: @panel-strong;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 5px 10px;
    min-width: 90px;
}
QComboBox:hover {
    border-color: @border-strong;
}
QComboBox::drop-down {
    border: none;
    width: 18px;
}
QComboBox QAbstractItemView {
    background: @bg-elevated;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 4px;
    selection-background-color: @sel-bg;
    selection-color: @text;
    outline: none;
}
QSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    width: 14px;
    background: transparent;
    border: none;
}
QCheckBox, QRadioButton {
    spacing: 8px;
}
QCheckBox::indicator, QRadioButton::indicator {
    width: 15px;
    height: 15px;
    border: 1px solid @border;
    background: @panel-strong;
    border-radius: 4px;
}
QRadioButton::indicator {
    border-radius: 8px;
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background: @accent;
    border-color: @accent;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border-color: @border-strong;
}

/* --- список треков (рисуется делегатом, здесь только рамка) ------------- */
QListWidget[dataUi="trackList"], QListWidget[dataUi="results"] {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget[dataUi="results"]::item {
    border-radius: @radius-sm;
    padding: 8px 10px;
    margin: 2px 4px;
    color: @text-muted;
}
QListWidget[dataUi="results"]::item:hover {
    background: @hover-bg;
    color: @text;
}
QListWidget[dataUi="results"]::item:selected {
    background: @sel-bg;
    color: @text;
    border: 1px solid @border-strong;
}

/* --- слайдеры, прогресс -------------------------------------------------- */
QSlider::groove:horizontal {
    height: 6px;
    background: @panel-solid;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background: @accent;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 14px;
    margin: -4px 0;
    border-radius: 7px;
    background: @text;
}
QSlider::handle:horizontal:hover {
    background: @accent;
}
QSlider::handle:horizontal:disabled {
    background: @text-faint;
}
QProgressBar {
    background: @panel-solid;
    border: 1px solid @border;
    border-radius: 8px;
    height: 16px;
    text-align: center;
    font-size: @font-size-sm;
    color: @text-muted;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 @accent, stop:1 @accent-hover);
    border-radius: 7px;
}

/* --- прокрутка ----------------------------------------------------------- */
QScrollBar:vertical {
    background: transparent;
    width: 12px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: @border-strong;
    border-radius: 6px;
    min-height: 36px;
}
QScrollBar::handle:vertical:hover {
    background: @accent;
}
QScrollBar:horizontal {
    background: transparent;
    height: 12px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background: @border-strong;
    border-radius: 6px;
    min-width: 36px;
}
QScrollBar::add-line, QScrollBar::sub-line {
    height: 0px;
    width: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

/* --- меню, подсказки, статус -------------------------------------------- */
QMenuBar {
    background: transparent;
    color: @text-muted;
}
QMenuBar::item {
    padding: 6px 10px;
    background: transparent;
    border-radius: @radius-sm;
}
QMenuBar::item:selected {
    background: @hover-bg;
    color: @text;
}
QMenu {
    background: @bg-elevated;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 6px;
}
QMenu::item {
    padding: 7px 22px 7px 14px;
    border-radius: @radius-sm;
    color: @text;
}
QMenu::item:selected {
    background: @sel-bg;
    color: @accent;
}
QMenu::separator {
    height: 1px;
    background: @border;
    margin: 5px 8px;
}
QToolTip {
    background: @bg-elevated;
    color: @text;
    border: 1px solid @border;
    border-radius: @radius-sm;
    padding: 5px 8px;
}
QStatusBar {
    background: transparent;
    color: @text-muted;
}
QStatusBar::item {
    border: none;
}

/* --- вкладки (диалог yt-dlp) -------------------------------------------- */
QTabWidget::pane {
    border: 1px solid @border;
    border-radius: @radius;
    background: @panel;
    top: -1px;
}
QTabBar::tab {
    background: transparent;
    color: @text-muted;
    padding: 7px 14px;
    margin-right: 4px;
    border-radius: @radius-sm;
}
QTabBar::tab:selected {
    background: @sel-bg;
    color: @text;
}
QTabBar::tab:hover {
    background: @hover-bg;
}

/* --- разделители -------------------------------------------------------- */
QSplitter::handle {
    background: @border;
}
QSplitter::handle:horizontal {
    width: 1px;
}
QScrollArea {
    border: none;
}
QFrame[dataUi="card"] {
    background: @panel;
    border: 1px solid @border;
    border-radius: @radius;
}
QFrame[dataUi="card"]:hover {
    border-color: @border-strong;
}
QFrame[dataUi="row"] {
    background: transparent;
    border: none;
}
QFrame[dataUi="hline"] {
    background: @border;
    max-height: 1px;
}
)QSS";

    return QString::fromUtf8(kTemplate);
}

BuildResult build(const Theme &theme, const ResolvedTokens &tokens, const QString &userCss,
                  bool customCssEnabled)
{
    BuildResult result;
    result.qss = substituteTokens(baseTemplate(), tokens.values);

    if (!theme.css.trimmed().isEmpty()) {
        QString themeCss = sanitizeUserCss(theme.css, theme.dirPath, &result.problems);
        result.qss += QStringLiteral("\n/* --- theme.qss: %1 --- */\n").arg(theme.id);
        result.qss += substituteTokens(themeCss, tokens.values);
    }

    if (customCssEnabled && !userCss.trimmed().isEmpty()) {
        QStringList problems;
        const QString cleaned = sanitizeUserCss(userCss, theme.dirPath, &problems);
        for (const QString &problem : problems)
            result.problems.append(QObject::tr("Свой CSS: %1").arg(problem));
        result.qss += QStringLiteral("\n/* --- свой CSS --- */\n");
        result.qss += substituteTokens(cleaned, tokens.values);
    }

    return result;
}

} // namespace StyleSheet