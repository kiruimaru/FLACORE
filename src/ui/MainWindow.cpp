#include "MainWindow.h"

#include "YouTubePanel.h"
#include "YtDlpSettingsDialog.h"
#include "core/ytdlp/YtDlp.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QUrl>

namespace {

const QList<QPair<QString, QString>> &themeColorRoles()
{
    static const QList<QPair<QString, QString>> roles = {
        {QStringLiteral("background"), QObject::tr("Фон окна")},
        {QStringLiteral("panel"), QObject::tr("Панели")},
        {QStringLiteral("input"), QObject::tr("Поля ввода")},
        {QStringLiteral("text"), QObject::tr("Основной текст")},
        {QStringLiteral("muted"), QObject::tr("Вторичный текст")},
        {QStringLiteral("border"), QObject::tr("Границы")},
        {QStringLiteral("borderStrong"), QObject::tr("Активные границы")},
        {QStringLiteral("accent"), QObject::tr("Акцент")},
        {QStringLiteral("selection"), QObject::tr("Выделение")},
        {QStringLiteral("cover"), QObject::tr("Обложка")}
    };
    return roles;
}

QString customColor(const QString &role, const QString &fallback)
{
    return QSettings().value(QStringLiteral("ui/customColors/") + role, fallback).toString();
}

QIcon interfaceIcon(const QString &name, QStyle::StandardPixmap fallback, QWidget *widget)
{
    const QIcon themed = QIcon::fromTheme(name);
    return themed.isNull() ? widget->style()->standardIcon(fallback) : themed;
}

QString trackCountSuffix(int count)
{
    const int lastTwoDigits = count % 100;
    const int lastDigit = count % 10;
    if (lastTwoDigits >= 11 && lastTwoDigits <= 14)
        return QObject::tr("треков");
    if (lastDigit == 1)
        return QObject::tr("трек");
    if (lastDigit >= 2 && lastDigit <= 4)
        return QObject::tr("трека");
    return QObject::tr("треков");
}

QString applicationStyle(const QString &theme)
{
    const bool lightTheme = theme == QLatin1String("light")
                            || theme == QLatin1String("apple")
                            || theme == QLatin1String("sunset");
    QString accent = QSettings().value(
        QStringLiteral("ui/customAccent"), QStringLiteral("#52d69a")).toString();
    QString background = lightTheme ? QStringLiteral("#f5f6f8") : QStringLiteral("#0d0f12");
    QString panel = lightTheme ? QStringLiteral("#ffffff") : QStringLiteral("#15181d");
    QString input = lightTheme ? QStringLiteral("#f0f2f5") : QStringLiteral("#1c2026");
    QString border = lightTheme ? QStringLiteral("#dfe3e8") : QStringLiteral("#292e36");
    QString borderStrong = lightTheme ? QStringLiteral("#cbd1d9") : QStringLiteral("#353c47");
    QString text = lightTheme ? QStringLiteral("#171a1f") : QStringLiteral("#f2f4f7");
    QString muted = lightTheme ? QStringLiteral("#69717d") : QStringLiteral("#929aa7");
    QString accentHover = accent;
    QString accentText = QStringLiteral("#08110d");
    if (theme == QLatin1String("soundcloud")) {
        accentHover = QStringLiteral("#ff8b42");
    } else if (theme == QLatin1String("apple")) {
        accentHover = QStringLiteral("#f04455");
        accentText = QStringLiteral("#ffffff");
    } else if (theme == QLatin1String("ocean")) {
        accentHover = QStringLiteral("#57c9e8");
        accentText = QStringLiteral("#071319");
    } else if (theme == QLatin1String("sunset")) {
        accentHover = QStringLiteral("#a04e36");
        accentText = QStringLiteral("#ffffff");
    } else if (theme != QLatin1String("custom")) {
        accentHover = QStringLiteral("#6be5ad");
    }
    if (theme == QLatin1String("soundcloud"))
        accent = QStringLiteral("#ff6b00");
    else if (theme == QLatin1String("apple"))
        accent = QStringLiteral("#e8334a");
    else if (theme == QLatin1String("ocean"))
        accent = QStringLiteral("#19a7ce");
    else if (theme == QLatin1String("sunset"))
        accent = QStringLiteral("#e57945");
    QString selection = lightTheme ? QStringLiteral("#d9f6e8") : QStringLiteral("#1c4438");
    QString cover = lightTheme ? QStringLiteral("#e9edf1") : QStringLiteral("#20262d");
    const QString danger = QStringLiteral("#f06b78");
    if (theme == QLatin1String("custom")) {
        background = customColor(QStringLiteral("background"), background);
        panel = customColor(QStringLiteral("panel"), panel);
        input = customColor(QStringLiteral("input"), input);
        text = customColor(QStringLiteral("text"), text);
        muted = customColor(QStringLiteral("muted"), muted);
        border = customColor(QStringLiteral("border"), border);
        borderStrong = customColor(QStringLiteral("borderStrong"), borderStrong);
        accent = customColor(QStringLiteral("accent"), accent);
        selection = customColor(QStringLiteral("selection"), selection);
        cover = customColor(QStringLiteral("cover"), cover);
        accentHover = QColor(accent).lighter(115).name();
        accentText = QColor(accent).lightness() > 150 ? QStringLiteral("#111111")
                                                       : QStringLiteral("#ffffff");
    }

    return QStringLiteral(R"(
QMainWindow {
    background: %1;
}

QWidget#appRoot {
    background: %19;
}

QWidget#topBar {
    background: transparent;
}

QLabel#brandLabel {
    color: %2;
    font-size: 23px;
    font-weight: 700;
}

QLabel#brandCaption,
QLabel#eyebrowLabel {
    color: %3;
    font-size: 10px;
    font-weight: 700;
}

QLabel#brandCaption {
    padding-top: 5px;
}

QLabel#statusPill {
    color: %4;
    background: %7;
    border: 1px solid %8;
    border-radius: 10px;
    padding: 5px 10px;
    font-size: 10px;
    font-weight: 700;
}

QWidget#playerPanel,
QWidget#libraryPanel {
    background: %5;
    border: 1px solid %8;
    border-radius: 14px;
}

QLabel#trackTitle {
    color: %2;
    font-size: 19px;
    font-weight: 700;
}

QLabel#trackPath,
QLabel#timeLabel,
QLabel#volumeLabel,
QLabel#playlistCount,
QLabel#emptyHint {
    color: %3;
}

QLabel#trackPath {
    font-size: 11px;
}

QFrame#coverArt {
    background: %9;
    border: 1px solid %8;
    border-radius: 14px;
}

QLabel#coverGlyph {
    color: %4;
    font-size: 76px;
    font-weight: 700;
}

QLabel#coverCaption {
    color: %3;
    font-size: 10px;
    font-weight: 700;
}

QPushButton {
    color: %2;
    background: %6;
    border: 1px solid %8;
    border-radius: 8px;
    padding: 8px 12px;
    min-height: 18px;
}

QPushButton:hover {
    background: %10;
    border-color: %11;
}

QPushButton:pressed {
    background: %12;
}

QPushButton:disabled {
    color: %13;
    background: %14;
    border-color: %15;
}

QPushButton#iconButton,
QPushButton#themeButton,
QPushButton#transportButton {
    padding: 0;
    min-width: 38px;
    min-height: 38px;
}

QPushButton#themeButton {
    min-width: 36px;
    max-width: 36px;
    min-height: 36px;
    max-height: 36px;
    border-radius: 18px;
    padding: 0;
    font-size: 18px;
}

QPushButton#primaryButton {
    color: %16;
    background: %4;
    border-color: %4;
    border-radius: 25px;
    padding: 0;
}

QPushButton#primaryButton:hover {
    background: %17;
    border-color: %17;
}

QPushButton#primaryButton:disabled {
    color: %13;
    background: %14;
    border-color: %15;
}

QPushButton#dangerButton {
    color: %18;
}

QLineEdit,
QComboBox {
    color: %2;
    background: %6;
    border: 1px solid %8;
    border-radius: 8px;
    padding: 8px 10px;
    selection-background-color: %4;
    selection-color: %16;
}

QLineEdit:focus,
QComboBox:focus {
    border-color: %4;
}

QComboBox {
    min-width: 110px;
}

QComboBox::drop-down {
    width: 26px;
    border: 0;
}

QComboBox QAbstractItemView {
    color: %2;
    background: %5;
    border: 1px solid %8;
    selection-background-color: %7;
    selection-color: %2;
}

QSlider::groove:horizontal {
    height: 4px;
    background: %14;
    border-radius: 2px;
}

QSlider::sub-page:horizontal {
    background: %4;
    border-radius: 2px;
}

QSlider::add-page:horizontal {
    background: %14;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    width: 13px;
    height: 13px;
    margin: -5px 0;
    background: %4;
    border: 2px solid %5;
    border-radius: 7px;
}

QSlider::handle:horizontal:hover {
    background: %17;
}

QTabWidget::pane {
    background: %5;
    border: 1px solid %8;
    border-radius: 10px;
    top: -1px;
}

QTabBar::tab {
    color: %3;
    background: transparent;
    border: 0;
    border-bottom: 2px solid transparent;
    padding: 9px 13px;
    margin-right: 4px;
}

QTabBar::tab:hover {
    color: %2;
}

QTabBar::tab:selected {
    color: %4;
    border-bottom-color: %4;
}

QListWidget {
    color: %2;
    background: %5;
    border: 0;
    outline: 0;
    padding: 6px;
}

QListWidget::item {
    background: transparent;
    border-radius: 7px;
    padding: 9px 10px;
    margin: 2px 0;
}

QListWidget::item:hover {
    background: %7;
}

QListWidget::item:selected {
    color: %2;
    background: %7;
    border: 1px solid %11;
}

QProgressBar {
    color: %3;
    background: %14;
    border: 0;
    border-radius: 4px;
    text-align: center;
    min-height: 8px;
    max-height: 8px;
}

QProgressBar::chunk {
    background: %4;
    border-radius: 4px;
}

QStatusBar {
    color: %3;
    background: %1;
    border-top: 1px solid %8;
}

QStatusBar::item {
    border: 0;
}

QMenuBar {
    color: %3;
    background: %1;
    border-bottom: 1px solid %8;
}

QMenuBar::item {
    background: transparent;
    padding: 6px 9px;
}

QMenuBar::item:selected,
QMenu::item:selected {
    color: %2;
    background: %7;
}

QMenu {
    color: %2;
    background: %5;
    border: 1px solid %8;
    padding: 5px;
}

QMenu::item {
    padding: 7px 28px 7px 10px;
    border-radius: 5px;
}

QToolTip {
    color: %2;
    background: %5;
    border: 1px solid %8;
    padding: 5px 7px;
}

QSplitter::handle {
    background: transparent;
}

QScrollBar:vertical {
    width: 8px;
    background: transparent;
    margin: 4px 0;
}

QScrollBar::handle:vertical {
    background: %8;
    border-radius: 4px;
    min-height: 30px;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0;
}

QLabel#ytStatus[error="true"] {
    color: %18;
}
)").arg(background, text, muted, accent, panel, input, selection, border,
        cover, lightTheme ? QStringLiteral("#f7f8fa") : QStringLiteral("#252b33"),
        borderStrong, lightTheme ? QStringLiteral("#e2e6eb") : QStringLiteral("#171a1f"),
        lightTheme ? QStringLiteral("#a4aab3") : QStringLiteral("#555d68"),
        lightTheme ? QStringLiteral("#e7e9ed") : QStringLiteral("#242a31"),
        border, accentText, accentHover, danger,
        QSettings().value(QStringLiteral("ui/backgroundImage")).toString().isEmpty()
            ? background : QStringLiteral("transparent"));
}

QPalette applicationPalette(const QString &theme)
{
    const bool lightTheme = theme == QLatin1String("light")
                            || theme == QLatin1String("apple")
                            || theme == QLatin1String("sunset");
    const QColor window = lightTheme ? QColor(QStringLiteral("#f5f6f8"))
                                     : QColor(QStringLiteral("#0d0f12"));
    const QColor panel = lightTheme ? QColor(QStringLiteral("#ffffff"))
                                    : QColor(QStringLiteral("#15181d"));
    const QColor input = lightTheme ? QColor(QStringLiteral("#f0f2f5"))
                                    : QColor(QStringLiteral("#1c2026"));
    const QColor text = lightTheme ? QColor(QStringLiteral("#171a1f"))
                                   : QColor(QStringLiteral("#f2f4f7"));
    const QColor muted = lightTheme ? QColor(QStringLiteral("#69717d"))
                                    : QColor(QStringLiteral("#929aa7"));
    QColor accent(QSettings().value(
        QStringLiteral("ui/customAccent"), QStringLiteral("#52d69a")).toString());
    if (theme == QLatin1String("soundcloud"))
        accent = QColor(QStringLiteral("#ff6b00"));
    else if (theme == QLatin1String("apple"))
        accent = QColor(QStringLiteral("#e8334a"));
    else if (theme == QLatin1String("ocean"))
        accent = QColor(QStringLiteral("#19a7ce"));
    else if (theme == QLatin1String("sunset"))
        accent = QColor(QStringLiteral("#e57945"));
    QColor selection = lightTheme ? QColor(QStringLiteral("#d9f6e8"))
                                  : QColor(QStringLiteral("#1c4438"));
    QColor actualWindow = window;
    QColor actualInput = input;
    QColor actualPanel = panel;
    QColor actualText = text;
    QColor actualMuted = muted;
    if (theme == QLatin1String("custom")) {
        actualWindow = QColor(customColor(QStringLiteral("background"), window.name()));
        actualPanel = QColor(customColor(QStringLiteral("panel"), panel.name()));
        actualInput = QColor(customColor(QStringLiteral("input"), input.name()));
        actualText = QColor(customColor(QStringLiteral("text"), text.name()));
        actualMuted = QColor(customColor(QStringLiteral("muted"), muted.name()));
        accent = QColor(customColor(QStringLiteral("accent"), accent.name()));
        selection = QColor(customColor(QStringLiteral("selection"), selection.name()));
    }

    QPalette palette;
    palette.setColor(QPalette::Window, actualWindow);
    palette.setColor(QPalette::Base, actualInput);
    palette.setColor(QPalette::AlternateBase, actualPanel);
    palette.setColor(QPalette::Button, actualInput);
    palette.setColor(QPalette::Text, actualText);
    palette.setColor(QPalette::WindowText, actualText);
    palette.setColor(QPalette::ButtonText, actualText);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#08110d")));
    palette.setColor(QPalette::PlaceholderText, actualMuted);
    palette.setColor(QPalette::ToolTipBase, actualPanel);
    palette.setColor(QPalette::ToolTipText, actualText);
    palette.setColor(QPalette::Mid, actualMuted);
    palette.setColor(QPalette::Disabled, QPalette::Text,
                     lightTheme ? QColor(QStringLiteral("#a4aab3"))
                                : QColor(QStringLiteral("#555d68")));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     lightTheme ? QColor(QStringLiteral("#a4aab3"))
                                : QColor(QStringLiteral("#555d68")));
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     lightTheme ? QColor(QStringLiteral("#a4aab3"))
                                : QColor(QStringLiteral("#555d68")));
    palette.setColor(QPalette::Inactive, QPalette::Highlight, selection);
    return palette;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);

    playlist = new Playlist(this);
    ytdlp = new YtDlp(this);

    m_theme = QSettings().value(QStringLiteral("ui/theme"),
                                QStringLiteral("dark")).toString();

    setupUI();
    createMenuBar();
    applyTheme(m_theme);

    connect(player, &QMediaPlayer::positionChanged,
            this, &MainWindow::onPositionChanged);
    connect(player, &QMediaPlayer::durationChanged,
            this, &MainWindow::onDurationChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status != QMediaPlayer::EndOfMedia)
                    return;

                if (playlist->hasNext()) {
                    onNext();
                    return;
                }

                playButton->setEnabled(playlist->count() > 0);
                pauseButton->setEnabled(false);
                playPauseStack->setCurrentWidget(playButton);
            });
    connect(playlist, &Playlist::trackAdded,
            this, &MainWindow::onPlaylistTrackAdded);
    connect(playlist, &Playlist::currentTrackChanged,
            this, &MainWindow::onCurrentTrackChanged);

    connect(youtubePanel, &YouTubePanel::playRequested,
            this, &MainWindow::playFile);
    connect(youtubePanel, &YouTubePanel::fileAdded, this,
            [this](const QString &path) {
                for (int i = 0; i < playlist->count(); ++i) {
                    if (playlist->getTrackPath(i) == path) {
                        statusBar()->showMessage(
                            tr("Уже в плейлисте: %1").arg(QFileInfo(path).fileName()),
                            5000);
                        return;
                    }
                }
                playlist->addTrack(path);
                updateButtonStates();
                statusBar()->showMessage(
                    tr("Добавлено в плейлист: %1").arg(QFileInfo(path).fileName()),
                    5000);
            });
    connect(youtubePanel, &YouTubePanel::filesAdded, this,
            [this](const QStringList &paths) {
                QStringList added;
                for (const QString &path : paths) {
                    if (!QFileInfo::exists(path))
                        continue;
                    bool exists = false;
                    for (int i = 0; i < playlist->count(); ++i) {
                        if (playlist->getTrackPath(i) == path) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                        added.append(path);
                }
                playlist->addTracks(added);
                updateButtonStates();
                statusBar()->showMessage(
                    tr("Добавлено в библиотеку и плейлист: %1 файлов").arg(added.size()),
                    6000);
            });

    connect(ytdlp, &YtDlp::availabilityChecked, this,
            [this](bool available, const QString &info) {
                statusBar()->showMessage(
                    available ? tr("yt-dlp %1 — готов").arg(info)
                              : tr("yt-dlp недоступен: %1").arg(info),
                    8000);
            });

    ytdlp->checkAvailability();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("appRoot"));
    centralWidget->setAutoFillBackground(true);
    setCentralWidget(centralWidget);

    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(QStringLiteral("FLACORE"));
    setMinimumSize(900, 680);
    resize(1180, 740);

    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(22, 18, 22, 10);
    mainLayout->setSpacing(14);

    auto *topBar = new QWidget(centralWidget);
    topBar->setObjectName(QStringLiteral("topBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(2, 0, 2, 0);
    topLayout->setSpacing(10);

    auto *brand = new QLabel(QStringLiteral("FLACORE"), topBar);
    brand->setObjectName(QStringLiteral("brandLabel"));
    auto *brandCaption = new QLabel(tr("ПЛЕЕР ДЛЯ МУЗЫКИ"), topBar);
    brandCaption->setObjectName(QStringLiteral("brandCaption"));

    topLayout->addWidget(brand);
    topLayout->addWidget(brandCaption);
    topLayout->addStretch();

    auto *statusPill = new QLabel(tr("АУДИО"), topBar);
    statusPill->setObjectName(QStringLiteral("statusPill"));
    topLayout->addWidget(statusPill);

    auto *feedbackButton = new QPushButton(tr("Feedback"), topBar);
    feedbackButton->setObjectName(QStringLiteral("feedbackButton"));
    feedbackButton->setToolTip(tr("Открыть GitHub Issues"));
    connect(feedbackButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral(
            "https://github.com/kiruimaru/FLACORE/issues")));
    });
    topLayout->addWidget(feedbackButton);

    themeButton = new QPushButton(topBar);
    themeButton->setObjectName(QStringLiteral("themeButton"));
    themeButton->setToolTip(tr("Переключить тему"));
    themeButton->setCursor(Qt::PointingHandCursor);
    themeButton->setFocusPolicy(Qt::NoFocus);
    connect(themeButton, &QPushButton::clicked,
            this, &MainWindow::onThemeToggle);
    topLayout->addWidget(themeButton);
    mainLayout->addWidget(topBar);

    auto *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    splitter->setObjectName(QStringLiteral("mainSplitter"));
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(10);
    mainLayout->addWidget(splitter, 1);

    auto *leftPanel = new QWidget(splitter);
    leftPanel->setObjectName(QStringLiteral("playerPanel"));
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 18, 20, 18);
    leftLayout->setSpacing(9);

    auto *nowPlayingRow = new QHBoxLayout();
    auto *nowPlayingLabel = new QLabel(tr("СЕЙЧАС ИГРАЕТ"), leftPanel);
    nowPlayingLabel->setObjectName(QStringLiteral("eyebrowLabel"));
    nowPlayingRow->addWidget(nowPlayingLabel);
    nowPlayingRow->addStretch();

    openButton = new QPushButton(leftPanel);
    openButton->setObjectName(QStringLiteral("iconButton"));
    openButton->setIcon(interfaceIcon(QStringLiteral("document-open"),
                                       QStyle::SP_DirOpenIcon, this));
    openButton->setToolTip(tr("Открыть аудиофайлы"));
    openButton->setAccessibleName(tr("Открыть аудиофайлы"));
    openButton->setCursor(Qt::PointingHandCursor);
    openButton->setFocusPolicy(Qt::NoFocus);
    connect(openButton, &QPushButton::clicked,
            this, &MainWindow::onOpenFiles);
    nowPlayingRow->addWidget(openButton);
    leftLayout->addLayout(nowPlayingRow);

    auto *coverArt = new QFrame(leftPanel);
    coverArt->setObjectName(QStringLiteral("coverArt"));
    coverArt->setFixedSize(220, 220);
    auto *coverLayout = new QVBoxLayout(coverArt);
    coverLayout->setContentsMargins(12, 12, 12, 12);
    coverLayout->addStretch();
    auto *coverGlyph = new QLabel(QStringLiteral("F"), coverArt);
    coverGlyph->setObjectName(QStringLiteral("coverGlyph"));
    coverGlyph->setAlignment(Qt::AlignCenter);
    coverLayout->addWidget(coverGlyph);
    auto *coverCaption = new QLabel(QStringLiteral("FLACORE"), coverArt);
    coverCaption->setObjectName(QStringLiteral("coverCaption"));
    coverCaption->setAlignment(Qt::AlignCenter);
    coverLayout->addWidget(coverCaption);
    coverLayout->addStretch();
    leftLayout->addWidget(coverArt, 0, Qt::AlignHCenter);

    fileLabel = new QLabel(QStringLiteral("Выберите трек"), leftPanel);
    fileLabel->setObjectName(QStringLiteral("trackTitle"));
    fileLabel->setAlignment(Qt::AlignCenter);
    fileLabel->setWordWrap(true);
    leftLayout->addWidget(fileLabel);

    filePathLabel = new QLabel(QStringLiteral("Откройте аудиофайл или найдите музыку онлайн"),
                               leftPanel);
    filePathLabel->setObjectName(QStringLiteral("trackPath"));
    filePathLabel->setAlignment(Qt::AlignCenter);
    filePathLabel->setWordWrap(true);
    leftLayout->addWidget(filePathLabel);

    progressSlider = new QSlider(Qt::Horizontal, leftPanel);
    progressSlider->setObjectName(QStringLiteral("seekSlider"));
    progressSlider->setRange(0, 0);
    progressSlider->setEnabled(false);
    connect(progressSlider, &QSlider::sliderMoved,
            this, &MainWindow::onSeek);
    leftLayout->addSpacing(5);
    leftLayout->addWidget(progressSlider);

    auto *timeLayout = new QHBoxLayout();
    currentTimeLabel = new QLabel(QStringLiteral("00:00"), leftPanel);
    currentTimeLabel->setObjectName(QStringLiteral("timeLabel"));
    totalTimeLabel = new QLabel(QStringLiteral("00:00"), leftPanel);
    totalTimeLabel->setObjectName(QStringLiteral("timeLabel"));
    timeLayout->addWidget(currentTimeLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(totalTimeLabel);
    leftLayout->addLayout(timeLayout);

    auto *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);
    controlLayout->addStretch();

    previousButton = new QPushButton(leftPanel);
    previousButton->setObjectName(QStringLiteral("transportButton"));
    previousButton->setIcon(interfaceIcon(QStringLiteral("media-skip-backward"),
                                           QStyle::SP_MediaSkipBackward, this));
    previousButton->setToolTip(tr("Предыдущий трек"));
    previousButton->setCursor(Qt::PointingHandCursor);
    previousButton->setFocusPolicy(Qt::NoFocus);
    previousButton->setEnabled(false);
    connect(previousButton, &QPushButton::clicked,
            this, &MainWindow::onPrevious);

    playButton = new QPushButton(leftPanel);
    playButton->setObjectName(QStringLiteral("primaryButton"));
    playButton->setIcon(interfaceIcon(QStringLiteral("media-play"),
                                      QStyle::SP_MediaPlay, this));
    playButton->setToolTip(tr("Воспроизвести"));
    playButton->setCursor(Qt::PointingHandCursor);
    playButton->setFocusPolicy(Qt::NoFocus);
    playButton->setEnabled(false);
    connect(playButton, &QPushButton::clicked,
            this, &MainWindow::onPlay);

    pauseButton = new QPushButton(leftPanel);
    pauseButton->setObjectName(QStringLiteral("primaryButton"));
    pauseButton->setIcon(interfaceIcon(QStringLiteral("media-playback-pause"),
                                        QStyle::SP_MediaPause, this));
    pauseButton->setToolTip(tr("Пауза"));
    pauseButton->setCursor(Qt::PointingHandCursor);
    pauseButton->setFocusPolicy(Qt::NoFocus);
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked,
            this, &MainWindow::onPause);

    playPauseStack = new QStackedWidget(leftPanel);
    playPauseStack->setObjectName(QStringLiteral("playPauseStack"));
    playPauseStack->setFixedSize(50, 50);
    playPauseStack->addWidget(playButton);
    playPauseStack->addWidget(pauseButton);
    playPauseStack->setCurrentWidget(playButton);

    stopButton = new QPushButton(leftPanel);
    stopButton->setObjectName(QStringLiteral("transportButton"));
    stopButton->setIcon(interfaceIcon(QStringLiteral("media-playback-stop"),
                                      QStyle::SP_MediaStop, this));
    stopButton->setToolTip(tr("Остановить"));
    stopButton->setCursor(Qt::PointingHandCursor);
    stopButton->setFocusPolicy(Qt::NoFocus);
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked,
            this, &MainWindow::onStop);

    nextButton = new QPushButton(leftPanel);
    nextButton->setObjectName(QStringLiteral("transportButton"));
    nextButton->setIcon(interfaceIcon(QStringLiteral("media-skip-forward"),
                                       QStyle::SP_MediaSkipForward, this));
    nextButton->setToolTip(tr("Следующий трек"));
    nextButton->setCursor(Qt::PointingHandCursor);
    nextButton->setFocusPolicy(Qt::NoFocus);
    nextButton->setEnabled(false);
    connect(nextButton, &QPushButton::clicked,
            this, &MainWindow::onNext);

    controlLayout->addWidget(previousButton);
    controlLayout->addWidget(playPauseStack);
    controlLayout->addWidget(stopButton);
    controlLayout->addWidget(nextButton);
    controlLayout->addStretch();
    leftLayout->addLayout(controlLayout);

    auto *volumeLayout = new QHBoxLayout();
    volumeLayout->setSpacing(9);
    auto *volumeLabel = new QLabel(QStringLiteral("VOL"), leftPanel);
    volumeLabel->setObjectName(QStringLiteral("volumeLabel"));
    volumeSlider = new QSlider(Qt::Horizontal, leftPanel);
    volumeSlider->setObjectName(QStringLiteral("volumeSlider"));
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    audioOutput->setVolume(0.5);
    connect(volumeSlider, &QSlider::valueChanged,
            this, &MainWindow::onVolumeChanged);
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(volumeSlider, 1);
    leftLayout->addLayout(volumeLayout);

    auto *rightPanel = new QWidget(splitter);
    rightPanel->setObjectName(QStringLiteral("libraryPanel"));
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(18, 18, 18, 18);
    rightLayout->setSpacing(10);

    auto *libraryHeader = new QHBoxLayout();
    auto *libraryTitle = new QLabel(tr("МОЯ БИБЛИОТЕКА"), rightPanel);
    libraryTitle->setObjectName(QStringLiteral("eyebrowLabel"));
    libraryHeader->addWidget(libraryTitle);
    libraryHeader->addStretch();
    playlistCountLabel = new QLabel(QStringLiteral("0 треков"), rightPanel);
    playlistCountLabel->setObjectName(QStringLiteral("playlistCount"));
    libraryHeader->addWidget(playlistCountLabel);
    rightLayout->addLayout(libraryHeader);

    playlistWidget = new QListWidget(rightPanel);
    playlistWidget->setObjectName(QStringLiteral("playlistWidget"));
    playlistWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistWidget->setUniformItemSizes(true);
    playlistWidget->setSpacing(2);
    playlistWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(playlistWidget, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onTrackDoubleClicked);
    connect(playlistWidget, &QListWidget::currentRowChanged,
            this, [this](int) { updateButtonStates(); });

    playlistStack = new QStackedWidget(rightPanel);
    playlistStack->setObjectName(QStringLiteral("playlistStack"));
    playlistStack->addWidget(playlistWidget);

    auto *emptyState = new QWidget(rightPanel);
    auto *emptyLayout = new QVBoxLayout(emptyState);
    emptyLayout->setContentsMargins(30, 20, 30, 20);
    emptyLayout->addStretch();
    auto *emptyMark = new QLabel(QStringLiteral("F"), emptyState);
    emptyMark->setObjectName(QStringLiteral("coverGlyph"));
    emptyMark->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyMark);
    auto *emptyTitle = new QLabel(QStringLiteral("Плейлист пуст"), emptyState);
    emptyTitle->setObjectName(QStringLiteral("trackTitle"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);
    auto *emptyHint = new QLabel(QStringLiteral("Добавьте локальные файлы через кнопку открытия"),
                                 emptyState);
    emptyHint->setObjectName(QStringLiteral("emptyHint"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyHint->setWordWrap(true);
    emptyLayout->addWidget(emptyHint);
    emptyLayout->addStretch();
    playlistStack->addWidget(emptyState);
    playlistStack->setCurrentWidget(emptyState);

    auto *playlistButtonLayout = new QHBoxLayout();
    playlistButtonLayout->setSpacing(8);
    removeTrackButton = new QPushButton(tr("Удалить"), rightPanel);
    removeTrackButton->setObjectName(QStringLiteral("dangerButton"));
    removeTrackButton->setIcon(interfaceIcon(QStringLiteral("edit-delete"),
                                              QStyle::SP_TrashIcon, this));
    removeTrackButton->setToolTip(tr("Удалить выбранный трек"));
    removeTrackButton->setEnabled(false);
    connect(removeTrackButton, &QPushButton::clicked,
            this, &MainWindow::onRemoveTrack);

    clearPlaylistButton = new QPushButton(tr("Очистить"), rightPanel);
    clearPlaylistButton->setIcon(interfaceIcon(QStringLiteral("edit-clear"),
                                                QStyle::SP_DialogResetButton, this));
    clearPlaylistButton->setToolTip(tr("Очистить плейлист"));
    clearPlaylistButton->setEnabled(false);
    connect(clearPlaylistButton, &QPushButton::clicked,
            this, &MainWindow::onClearPlaylist);
    playlistButtonLayout->addWidget(removeTrackButton);
    playlistButtonLayout->addWidget(clearPlaylistButton);
    playlistButtonLayout->addStretch();

    auto *playlistTab = new QWidget(rightPanel);
    auto *playlistTabLayout = new QVBoxLayout(playlistTab);
    playlistTabLayout->setContentsMargins(0, 0, 0, 0);
    playlistTabLayout->addWidget(playlistStack, 1);
    playlistTabLayout->addLayout(playlistButtonLayout);

    auto *rightTabs = new QTabWidget(rightPanel);
    rightTabs->setObjectName(QStringLiteral("contentTabs"));
    youtubePanel = new YouTubePanel(ytdlp, this);
    rightTabs->addTab(playlistTab,
                      interfaceIcon(QStringLiteral("audio-x-generic"),
                                    QStyle::SP_MediaVolume, this),
                      tr("Плейлист"));
    rightTabs->addTab(youtubePanel,
                      interfaceIcon(QStringLiteral("system-search"),
                                    QStyle::SP_FileDialogContentsView, this),
                      tr("Онлайн"));
    rightLayout->addWidget(rightTabs, 1);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 6);
    splitter->setSizes(QList<int>{430, 650});

    statusBar()->setObjectName(QStringLiteral("appStatusBar"));
    statusBar()->showMessage(tr("Готово"));
}

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));

    QAction *openAction = new QAction(tr("Открыть файлы"), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered,
            this, &MainWindow::onOpenFiles);
    fileMenu->addAction(openAction);

    QAction *quitAction = new QAction(tr("Выйти"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered,
            this, &QMainWindow::close);
    fileMenu->addAction(quitAction);

    QMenu *toolsMenu = menuBar()->addMenu(tr("Инструменты"));

    QAction *openYtDlpSettings = new QAction(tr("Настройки yt-dlp..."), this);
    connect(openYtDlpSettings, &QAction::triggered,
            this, &MainWindow::onYtDlpSettings);
    toolsMenu->addAction(openYtDlpSettings);

    QAction *openLibrary = new QAction(tr("Открыть папку библиотеки"), this);
    connect(openLibrary, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(YtDlp::libraryDir()));
    });
    toolsMenu->addAction(openLibrary);

    QMenu *viewMenu = menuBar()->addMenu(tr("Вид"));
    QMenu *themeMenu = viewMenu->addMenu(tr("Тема"));
    auto *themeGroup = new QActionGroup(themeMenu);
    themeGroup->setExclusive(true);

    darkThemeAction = themeMenu->addAction(tr("Тёмная"));
    darkThemeAction->setCheckable(true);
    darkThemeAction->setActionGroup(themeGroup);
    darkThemeAction->setData(QStringLiteral("dark"));
    m_themeActions.append(darkThemeAction);
    lightThemeAction = themeMenu->addAction(tr("Светлая"));
    lightThemeAction->setCheckable(true);
    lightThemeAction->setActionGroup(themeGroup);
    lightThemeAction->setData(QStringLiteral("light"));
    m_themeActions.append(lightThemeAction);
    const QList<QPair<QString, QString>> presets = {
        {QStringLiteral("soundcloud"), tr("SoundCloud")},
        {QStringLiteral("apple"), tr("Apple Music")},
        {QStringLiteral("ocean"), tr("Океан")},
        {QStringLiteral("sunset"), tr("Закат")}
    };
    for (const auto &preset : presets) {
        QAction *action = themeMenu->addAction(preset.second);
        action->setCheckable(true);
        action->setActionGroup(themeGroup);
        action->setData(preset.first);
        m_themeActions.append(action);
        connect(action, &QAction::triggered, this,
                [this, id = preset.first]() { applyTheme(id); });
    }
    themeMenu->addSeparator();
    QAction *customAccent = themeMenu->addAction(tr("Настроить акцентный цвет..."));
    connect(customAccent, &QAction::triggered, this, [this]() {
        const QColor current(QSettings().value(
            QStringLiteral("ui/customAccent"), QStringLiteral("#52d69a")).toString());
        const QColor color = QColorDialog::getColor(current, this, tr("Акцентный цвет"));
        if (!color.isValid())
            return;
        QSettings().setValue(QStringLiteral("ui/customAccent"), color.name());
        applyTheme(QStringLiteral("custom"));
    });
    QAction *hardcore = themeMenu->addAction(tr("Хардкор: настроить все цвета..."));
    connect(hardcore, &QAction::triggered, this, &MainWindow::editThemeHardcore);
    QAction *interactive = themeMenu->addAction(tr("Интерактивная настройка..."));
    connect(interactive, &QAction::triggered, this, &MainWindow::editThemeInteractive);
    themeMenu->addSeparator();
    QAction *exportConfig = themeMenu->addAction(tr("Экспорт цветового конфига..."));
    connect(exportConfig, &QAction::triggered, this, &MainWindow::exportThemeConfig);
    QAction *importConfig = themeMenu->addAction(tr("Импорт цветового конфига..."));
    connect(importConfig, &QAction::triggered, this, &MainWindow::importThemeConfig);
    QAction *setBackground = themeMenu->addAction(tr("Установить фото фоном..."));
    connect(setBackground, &QAction::triggered, this, &MainWindow::chooseBackground);
    QAction *clearBackground = themeMenu->addAction(tr("Убрать фото фона"));
    connect(clearBackground, &QAction::triggered, this, [this]() {
        QSettings().remove(QStringLiteral("ui/backgroundImage"));
        m_backgroundImage = QPixmap();
        applyTheme(m_theme);
    });

    connect(darkThemeAction, &QAction::triggered,
            this, [this]() { applyTheme(QStringLiteral("dark")); });
    connect(lightThemeAction, &QAction::triggered,
            this, [this]() { applyTheme(QStringLiteral("light")); });
}

void MainWindow::applyTheme(const QString &theme)
{
    m_theme = theme;
    QApplication::setPalette(applicationPalette(theme));
    qApp->setStyleSheet(applicationStyle(theme));

    QSettings settings;
    settings.setValue(QStringLiteral("ui/theme"), theme);
    settings.sync();

    const QString imagePath = settings.value(QStringLiteral("ui/backgroundImage")).toString();
    m_backgroundImage = imagePath.isEmpty() ? QPixmap() : QPixmap(imagePath);
    applyBackground();
    updateThemeButton();
}

void MainWindow::applyBackground()
{
    if (centralWidget == nullptr)
        return;
    QPalette palette = centralWidget->palette();
    if (m_backgroundImage.isNull()) {
        palette.setBrush(QPalette::Window, QApplication::palette().brush(QPalette::Window));
    } else {
        const QSize target = centralWidget->size();
        const QPixmap scaled = m_backgroundImage.scaled(
            target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const QRect crop((scaled.width() - target.width()) / 2,
                         (scaled.height() - target.height()) / 2,
                         target.width(), target.height());
        palette.setBrush(QPalette::Window, QBrush(scaled.copy(crop)));
    }
    centralWidget->setPalette(palette);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    applyBackground();
}

void MainWindow::editThemeHardcore()
{
    QSettings settings;
    const QMap<QString, QString> defaults = {
        {QStringLiteral("background"), QStringLiteral("#0d0f12")},
        {QStringLiteral("panel"), QStringLiteral("#15181d")},
        {QStringLiteral("input"), QStringLiteral("#1c2026")},
        {QStringLiteral("text"), QStringLiteral("#f2f4f7")},
        {QStringLiteral("muted"), QStringLiteral("#929aa7")},
        {QStringLiteral("border"), QStringLiteral("#292e36")},
        {QStringLiteral("borderStrong"), QStringLiteral("#353c47")},
        {QStringLiteral("accent"), QStringLiteral("#52d69a")},
        {QStringLiteral("selection"), QStringLiteral("#1c4438")},
        {QStringLiteral("cover"), QStringLiteral("#20262d")}
    };
    for (const auto &role : themeColorRoles()) {
        const QString key = QStringLiteral("ui/customColors/") + role.first;
        const QColor current(settings.value(key, defaults.value(role.first)).toString());
        const QColor color = QColorDialog::getColor(current, this, role.second);
        if (!color.isValid())
            return;
        settings.setValue(key, color.name());
    }
    applyTheme(QStringLiteral("custom"));
}

void MainWindow::editThemeInteractive()
{
    QSettings settings;
    for (const auto &role : themeColorRoles()) {
        QString description;
        if (role.first == QLatin1String("background"))
            description = tr("Основной цвет свободного пространства окна.");
        else if (role.first == QLatin1String("panel"))
            description = tr("Заливка панели плеера и панели библиотеки.");
        else if (role.first == QLatin1String("input"))
            description = tr("Поля поиска, ввода и обычные кнопки.");
        else if (role.first == QLatin1String("text"))
            description = tr("Заголовки, основной текст и подписи.");
        else if (role.first == QLatin1String("muted"))
            description = tr("Подсказки, вторичные подписи и неактивный текст.");
        else if (role.first == QLatin1String("border"))
            description = tr("Тонкие границы панелей и элементов.");
        else if (role.first == QLatin1String("borderStrong"))
            description = tr("Границы при наведении и выделении.");
        else if (role.first == QLatin1String("accent"))
            description = tr("Активные кнопки, ползунки и индикаторы.");
        else if (role.first == QLatin1String("selection"))
            description = tr("Фон выбранного трека и раскрытых меню.");
        else
            description = tr("Фон обложки в панели плеера.");
        QMessageBox prompt(QMessageBox::Question, tr("Интерактивная тема"),
                           tr("Цветовая роль: %1").arg(role.second),
                           QMessageBox::Ok | QMessageBox::Cancel, this);
        prompt.setInformativeText(description + QLatin1Char('\n')
                                  + tr("Выберите цвет, чтобы сразу увидеть его в интерфейсе."));
        if (prompt.exec() != QMessageBox::Ok)
            return;
        const QString key = QStringLiteral("ui/customColors/") + role.first;
        const QColor current(settings.value(key, QStringLiteral("#20262d")).toString());
        const QColor color = QColorDialog::getColor(current, this, role.second);
        if (!color.isValid())
            return;
        settings.setValue(key, color.name());
        applyTheme(QStringLiteral("custom"));
    }
}

void MainWindow::exportThemeConfig()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Экспорт темы"), QStringLiteral("flacore-theme.json"),
        tr("Цветовой конфиг (*.json)"));
    if (path.isEmpty())
        return;
    QJsonObject colors;
    for (const auto &role : themeColorRoles())
        colors.insert(role.first, customColor(role.first, QStringLiteral("#20262d")));
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(QJsonObject{{QStringLiteral("colors"), colors}})
                       .toJson(QJsonDocument::Indented));
        statusBar()->showMessage(tr("Цветовой конфиг экспортирован"), 4000);
    }
}

void MainWindow::importThemeConfig()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Импорт темы"), QString(), tr("Цветовой конфиг (*.json)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()
        || !document.object().value(QStringLiteral("colors")).isObject()) {
        QMessageBox::warning(this, tr("Ошибка конфига"), tr("Файл цветовой темы повреждён."));
        return;
    }
    const QJsonObject colors = document.object().value(QStringLiteral("colors")).toObject();
    QSettings settings;
    for (const auto &role : themeColorRoles()) {
        const QJsonValue value = colors.value(role.first);
        if (!value.isString())
            continue;
        const QColor color(value.toString());
        if (color.isValid())
            settings.setValue(QStringLiteral("ui/customColors/") + role.first, color.name());
    }
    applyTheme(QStringLiteral("custom"));
}

void MainWindow::chooseBackground()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Фото фона"), QString(),
        tr("Изображения (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty())
        return;
    QPixmap image(path);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Ошибка изображения"), tr("Не удалось открыть изображение."));
        return;
    }
    QSettings().setValue(QStringLiteral("ui/backgroundImage"), path);
    m_backgroundImage = image;
    applyBackground();
}

void MainWindow::updateThemeButton()
{
    if (themeButton == nullptr)
        return;

    const QIcon icon = QIcon::fromTheme(
        (m_theme == QLatin1String("light") || m_theme == QLatin1String("apple")
         || m_theme == QLatin1String("sunset"))
            ? QStringLiteral("weather-clear-night")
            : QStringLiteral("weather-clear"));
    themeButton->setIcon(icon);
    themeButton->setText(icon.isNull()
                             ? ((m_theme == QLatin1String("light")
                                 || m_theme == QLatin1String("apple")
                                 || m_theme == QLatin1String("sunset"))
                                    ? QStringLiteral("☾") : QStringLiteral("☼"))
                             : QString());
    const bool lightTheme = m_theme == QLatin1String("light")
                            || m_theme == QLatin1String("apple")
                            || m_theme == QLatin1String("sunset");
    themeButton->setAccessibleName(lightTheme
                                       ? tr("Переключить на тёмную тему")
                                       : tr("Переключить на светлую тему"));
    themeButton->setToolTip(lightTheme
                                ? tr("Переключить на тёмную тему")
                                : tr("Переключить на светлую тему"));

    for (QAction *action : m_themeActions)
        action->setChecked(action->data().toString() == m_theme);
}

void MainWindow::onThemeToggle()
{
    const bool lightTheme = m_theme == QLatin1String("light")
                            || m_theme == QLatin1String("apple")
                            || m_theme == QLatin1String("sunset");
    applyTheme(lightTheme ? QStringLiteral("dark") : QStringLiteral("light"));
}

void MainWindow::onOpenFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Выберите аудиофайлы"), QDir::homePath(),
        tr("Аудиофайлы (*.mp3 *.wav *.flac *.ogg *.m4a *.aac)"));

    if (!files.isEmpty()) {
        playlist->addTracks(files);
        updateButtonStates();
        statusBar()->showMessage(tr("Добавлено файлов: %1").arg(files.size()), 4000);
    }
}

void MainWindow::onPlay()
{
    if (player->mediaStatus() == QMediaPlayer::EndOfMedia) {
        player->setPosition(0);
        progressSlider->setValue(0);
        currentTimeLabel->setText(QStringLiteral("00:00"));
    }
    player->play();
    playButton->setEnabled(false);
    pauseButton->setEnabled(true);
    playPauseStack->setCurrentWidget(pauseButton);
}

void MainWindow::onPause()
{
    player->pause();
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    playPauseStack->setCurrentWidget(playButton);
}

void MainWindow::onStop()
{
    player->stop();
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    playPauseStack->setCurrentWidget(playButton);
    progressSlider->setValue(0);
}

void MainWindow::onNext()
{
    if (playlist->hasNext()) {
        playlist->next();
        loadTrack(playlist->currentIndex());
        player->play();
        playButton->setEnabled(false);
        pauseButton->setEnabled(true);
        playPauseStack->setCurrentWidget(pauseButton);
    }
}

void MainWindow::onPrevious()
{
    if (playlist->hasPrevious()) {
        playlist->previous();
        loadTrack(playlist->currentIndex());
        player->play();
        playButton->setEnabled(false);
        pauseButton->setEnabled(true);
        playPauseStack->setCurrentWidget(pauseButton);
    }
}

void MainWindow::onTrackDoubleClicked(QListWidgetItem *item)
{
    const int index = playlistWidget->row(item);
    playlist->setCurrentIndex(index);
    loadTrack(index);
    player->play();
    playButton->setEnabled(false);
    pauseButton->setEnabled(true);
    playPauseStack->setCurrentWidget(pauseButton);
}

void MainWindow::onRemoveTrack()
{
    const int currentRow = playlistWidget->currentRow();
    if (currentRow < 0)
        return;

    playlist->removeTrack(currentRow);
    delete playlistWidget->takeItem(currentRow);
    updateButtonStates();
}

void MainWindow::onClearPlaylist()
{
    playlist->clear();
    playlistWidget->clear();
    player->stop();
    fileLabel->setText(tr("Выберите трек"));
    filePathLabel->setText(tr("Откройте аудиофайл или найдите музыку онлайн"));
    currentTimeLabel->setText(QStringLiteral("00:00"));
    totalTimeLabel->setText(QStringLiteral("00:00"));
    progressSlider->setRange(0, 0);
    progressSlider->setValue(0);
    progressSlider->setEnabled(false);
    playButton->setEnabled(false);
    pauseButton->setEnabled(false);
    playPauseStack->setCurrentWidget(playButton);
    updateButtonStates();
    statusBar()->showMessage(tr("Плейлист очищен"), 3000);
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (!progressSlider->isSliderDown())
        progressSlider->setValue(static_cast<int>(position));

    const int seconds = static_cast<int>((position / 1000) % 60);
    const int minutes = static_cast<int>((position / 1000) / 60);
    currentTimeLabel->setText(QString::asprintf("%02d:%02d", minutes, seconds));
}

void MainWindow::onDurationChanged(qint64 duration)
{
    progressSlider->setRange(0, static_cast<int>(duration));
    progressSlider->setEnabled(duration > 0);

    const int seconds = static_cast<int>((duration / 1000) % 60);
    const int minutes = static_cast<int>((duration / 1000) / 60);
    totalTimeLabel->setText(QString::asprintf("%02d:%02d", minutes, seconds));
}

void MainWindow::onVolumeChanged(int value)
{
    audioOutput->setVolume(value / 100.0);
}

void MainWindow::onSeek(int position)
{
    player->setPosition(position);
}

void MainWindow::onPlaylistTrackAdded(int index)
{
    const Track track = playlist->getTrack(index);
    auto *item = new QListWidgetItem(track.title, playlistWidget);
    item->setToolTip(track.filePath);
    item->setSizeHint(QSize(0, 42));
    updateButtonStates();
}

void MainWindow::onCurrentTrackChanged(int index)
{
    playlistWidget->setCurrentRow(index);
    updateButtonStates();
}

void MainWindow::loadTrack(int index)
{
    const QString path = playlist->getTrackPath(index);
    if (path.isEmpty())
        return;

    player->setSource(QUrl::fromLocalFile(path));
    const Track track = playlist->getTrack(index);
    fileLabel->setText(track.title);
    filePathLabel->setText(QFileInfo(path).absolutePath());
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(true);
    playPauseStack->setCurrentWidget(playButton);
}

void MainWindow::updateButtonStates()
{
    const bool hasItems = playlist->count() > 0;
    playButton->setEnabled(hasItems);
    stopButton->setEnabled(hasItems);
    nextButton->setEnabled(playlist->hasNext());
    previousButton->setEnabled(playlist->hasPrevious());
    removeTrackButton->setEnabled(playlistWidget->currentRow() >= 0);
    clearPlaylistButton->setEnabled(hasItems);

    if (playlistCountLabel != nullptr) {
        playlistCountLabel->setText(
            tr("%1 %2")
                .arg(playlist->count())
                .arg(trackCountSuffix(playlist->count())));
    }
    if (playlistStack != nullptr)
        playlistStack->setCurrentIndex(hasItems ? 0 : 1);
}

void MainWindow::playFile(const QString &path)
{
    if (path.isEmpty())
        return;

    int index = -1;
    for (int i = 0; i < playlist->count(); ++i) {
        if (playlist->getTrackPath(i) == path) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        playlist->addTrack(path);
        index = playlist->count() - 1;
    }

    playlist->setCurrentIndex(index);
    loadTrack(index);
    player->play();
    playButton->setEnabled(false);
    pauseButton->setEnabled(true);
    playPauseStack->setCurrentWidget(pauseButton);
    progressSlider->setValue(0);
}

void MainWindow::onYtDlpSettings()
{
    YtDlpSettingsDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QSettings settings;
    settings.setValue(QStringLiteral("ytdlp/binary"), dialog.binaryPath());
    settings.setValue(QStringLiteral("ytdlp/ffmpegDir"), dialog.ffmpegDir());
    settings.setValue(QStringLiteral("ytdlp/libraryDir"), dialog.libraryDir());
    settings.setValue(QStringLiteral("ytdlp/audioFormat"), dialog.audioFormat());
    settings.sync();

    ytdlp->setBinary(dialog.binaryPath());
    ytdlp->checkAvailability();
    statusBar()->showMessage(tr("Настройки yt-dlp сохранены"), 4000);
}
