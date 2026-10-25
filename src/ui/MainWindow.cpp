#include "MainWindow.h"

#include "pages/AppearancePage.h"
#include "pages/DiscoverPage.h"
#include "pages/LibraryPage.h"
#include "pages/YtDlpPage.h"
#include "theme/ThemeManager.h"
#include "theme/ThemeUtil.h"
#include "widgets/AuroraBackdrop.h"
#include "widgets/GlassWidgets.h"
#include "widgets/Icons.h"
#include "widgets/PlayerBar.h"
#include "widgets/TitleBar.h"
#include "YouTubePanel.h"
#include "YtDlpSettingsDialog.h"
#include "YtDlpSettingsForm.h"

#include "core/online/Charts.h"
#include "core/player/Playlist.h"
#include "core/ytdlp/YtDlp.h"

#include <QAction>
#include <QAudioOutput>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

namespace {

constexpr int kTopInset = 12;

// Разделы слева: порядок совпадает с порядком страниц в QStackedWidget.
constexpr int kDiscoverRow = 0;
constexpr int kLibraryRow = 1;
constexpr int kYouTubeRow = 2;
constexpr int kAppearanceRow = 3;
constexpr int kYtDlpRow = 4;
constexpr int kNavCount = 5;

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("FLACORE"));
    resize(1180, 730);
    setMinimumSize(940, 600);

    m_media = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_media->setAudioOutput(m_audio);
    m_playlist = new Playlist(this);
    m_ytdlp = new YtDlp(this);

    setupUi();
    setupActions();
    wireCore();

    // Громкость запоминается между запусками.
    const int volume = QSettings().value(QStringLiteral("player/volume"), 70).toInt();
    m_audio->setVolume(volume / 100.0);
    m_playerBar->setVolume(volume);

    applyFrameless(AppearancePage::framelessEnabled());
    updateNavIcons();
    refreshTransport();

    m_ytdlp->checkAvailability();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    m_backdrop = new AuroraBackdrop(this);
    m_backdrop->setProperty("dataUi", "app");
    setCentralWidget(m_backdrop);

    auto *outer = new QVBoxLayout(m_backdrop);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_titleBar = new TitleBar(m_backdrop);
    outer->addWidget(m_titleBar);

    auto *middle = new QHBoxLayout();
    middle->setContentsMargins(0, 0, 0, 0);
    middle->setSpacing(0);

    // --- сайдбар -----------------------------------------------------------
    auto *sidebar = new QWidget(m_backdrop);
    sidebar->setProperty("dataUi", "sidebar");
    sidebar->setFixedWidth(ThemeUtil::number(QStringLiteral("sidebar-width"), 216));

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(8, 10, 8, 10);
    sidebarLayout->setSpacing(8);

    m_nav = new QListWidget(sidebar);
    m_nav->setProperty("dataUi", "sidebar");
    m_nav->setObjectName(QStringLiteral("sidebarNav"));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setFocusPolicy(Qt::NoFocus);
    m_nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_nav->addItem(tr("Обзор"));
    m_nav->addItem(tr("Библиотека"));
    m_nav->addItem(tr("YouTube"));
    m_nav->addItem(tr("Оформление"));
    m_nav->addItem(tr("yt-dlp"));
    m_nav->setCurrentRow(0);
    sidebarLayout->addWidget(m_nav, 1);

    auto *feedbackButton = new QPushButton(tr("Feedback"), sidebar);
    feedbackButton->setObjectName(QStringLiteral("feedbackButton"));
    feedbackButton->setProperty("variant", "ghost");
    feedbackButton->setToolTip(tr("Открыть GitHub Issues"));
    feedbackButton->setIcon(Icons::icon(Icons::Kind::Link,
                                        ThemeUtil::color(QStringLiteral("text-muted"), Qt::gray), 16));
    connect(feedbackButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/kiruimaru/FLACORE/issues")));
    });
    sidebarLayout->addWidget(feedbackButton);

    auto *menuButton = new QPushButton(tr("Меню"), sidebar);
    menuButton->setProperty("variant", "ghost");
    menuButton->setIcon(Icons::icon(Icons::Kind::Menu,
                                    ThemeUtil::color(QStringLiteral("text-muted"), Qt::gray), 16));
    sidebarLayout->addWidget(menuButton);

    middle->addWidget(sidebar);

    // --- страницы ----------------------------------------------------------
    m_pages = new QStackedWidget(m_backdrop);
    m_pages->setProperty("dataUi", "main");

    m_discover = new DiscoverPage(m_pages);
    m_library = new LibraryPage(m_pages);
    m_youtube = new YouTubePanel(m_ytdlp, m_pages);
    m_appearance = new AppearancePage(m_pages);
    m_ytdlpPage = new YtDlpPage(m_pages);

    m_pages->addWidget(m_discover);
    m_pages->addWidget(m_library);
    m_pages->addWidget(m_youtube);
    m_pages->addWidget(m_appearance);
    m_pages->addWidget(m_ytdlpPage);

    middle->addWidget(m_pages, 1);
    outer->addLayout(middle, 1);

    // --- панель плеера -----------------------------------------------------
    auto *playerWrap = new QWidget(m_backdrop);
    auto *playerLayout = new QVBoxLayout(playerWrap);
    playerLayout->setContentsMargins(16, 6, 16, 14);
    playerLayout->setSpacing(0);

    m_playerBar = new PlayerBar(playerWrap);
    playerLayout->addWidget(m_playerBar);
    outer->addWidget(playerWrap);

    // --- всплывающие сообщения --------------------------------------------
    m_toast = new QLabel(m_backdrop);
    m_toast->setProperty("dataUi", "toast");
    m_toast->setVisible(false);
    m_toast->raise();

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    m_toastTimer->setInterval(6000);
    connect(m_toastTimer, &QTimer::timeout, m_toast, &QLabel::hide);

    connect(menuButton, &QPushButton::clicked, this, [this, menuButton]() {
        onShowMenu(menuButton->mapToGlobal(QPoint(0, -8)));
    });
}

void MainWindow::setupActions()
{
    auto addWindowAction = [this](const QString &text, const QKeySequence &shortcut,
                                 const std::function<void()> &handler) -> QAction * {
        auto *action = new QAction(text, this);
        if (!shortcut.isEmpty())
            action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, handler);
        addAction(action);
        return action;
    };

    addWindowAction(tr("Открыть файлы"), QKeySequence::Open, [this]() { onOpenFiles(); });
    addWindowAction(tr("Играть / пауза"), QKeySequence(Qt::Key_Space), [this]() { onPlayPause(); });
    addWindowAction(tr("Следующий трек"), QKeySequence(Qt::CTRL | Qt::Key_Right),
                    [this]() { onNext(); });
    addWindowAction(tr("Предыдущий трек"), QKeySequence(Qt::CTRL | Qt::Key_Left),
                    [this]() { onPrevious(); });
    addWindowAction(tr("Стоп"), QKeySequence(Qt::CTRL | Qt::Key_Period), [this]() { onStop(); });

    // Аварийный выход из «своего CSS» — как Ctrl+Alt+Shift+C в SoundCloud Desktop.
    addWindowAction(tr("Выключить свой CSS и тему"),
                    QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_C), [this]() {
                        ThemeManager::instance()->toggleEscapeHatch();
                        notify(tr("Оформление переключено в безопасный вид (Ctrl+Alt+Shift+C)."));
                    });
}

void MainWindow::wireCore()
{
    ThemeManager *themes = ThemeManager::instance();
    if (themes) {
        connect(themes, &ThemeManager::changed, this, &MainWindow::updateNavIcons);
        connect(themes, &ThemeManager::problemsChanged, this, [this](const QStringList &problems) {
            if (!problems.isEmpty())
                notify(problems.join(QStringLiteral(" · ")), true);
        });
    }

    // --- шапка -------------------------------------------------------------
    connect(m_titleBar, &TitleBar::searchChanged, this, [this](const QString &text) {
        m_library->setFilter(text);
    });
    connect(m_titleBar, &TitleBar::appearanceRequested, this, [this]() {
        m_nav->setCurrentRow(kAppearanceRow);
    });
    connect(m_titleBar, &TitleBar::menuRequested, this, &MainWindow::onShowMenu);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, [this]() { showMinimized(); });
    connect(m_titleBar, &TitleBar::maximizeRequested, this, &MainWindow::toggleMaximize);
    connect(m_titleBar, &TitleBar::closeRequested, this, [this]() { close(); });

    connect(m_nav, &QListWidget::currentRowChanged, this, &MainWindow::onNavChanged);

    // --- библиотека --------------------------------------------------------
    connect(m_library, &LibraryPage::openFilesRequested, this, &MainWindow::onOpenFiles);
    connect(m_library, &LibraryPage::clearRequested, this, [this]() {
        m_playlist->clear();
        notify(tr("Плейлист очищен"));
    });
    connect(m_library, &LibraryPage::removeRequested, this, [this](int row) {
        if (row >= 0)
            m_playlist->removeTrack(row);
    });
    connect(m_library, &LibraryPage::trackActivated, this, &MainWindow::onTrackActivated);
    connect(m_library, &LibraryPage::filesDropped, this, [this](const QStringList &paths) {
        addPaths(paths, true);
    });

    // --- плеер -------------------------------------------------------------
    connect(m_playerBar, &PlayerBar::playPauseRequested, this, &MainWindow::onPlayPause);
    connect(m_playerBar, &PlayerBar::stopRequested, this, &MainWindow::onStop);
    connect(m_playerBar, &PlayerBar::nextRequested, this, &MainWindow::onNext);
    connect(m_playerBar, &PlayerBar::previousRequested, this, &MainWindow::onPrevious);
    connect(m_playerBar, &PlayerBar::seekRequested, this, &MainWindow::onSeek);
    connect(m_playerBar, &PlayerBar::volumeChanged, this, &MainWindow::onVolumeChanged);

    // --- модель ------------------------------------------------------------
    connect(m_playlist, &Playlist::trackAdded, this, &MainWindow::onPlaylistTrackAdded);
    connect(m_playlist, &Playlist::trackRemoved, this, &MainWindow::onPlaylistTrackRemoved);
    connect(m_playlist, &Playlist::currentTrackChanged, this, &MainWindow::onCurrentTrackChanged);
    connect(m_playlist, &Playlist::playlistCleared, this, [this]() {
        m_library->clearRows();
        m_media->stop();
        m_playingIndex = -1;
        m_playerBar->clearTrack();
        refreshTransport();
    });

    // --- медиа -------------------------------------------------------------
    connect(m_media, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_media, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_media, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(m_media, &QMediaPlayer::errorOccurred, this, &MainWindow::onMediaError);
    connect(m_media, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                m_playerBar->setPlaying(state == QMediaPlayer::PlayingState);
            });

    // --- yt-dlp ------------------------------------------------------------
    connect(m_ytdlp, &YtDlp::availabilityChecked, this, [this](bool available, const QString &info) {
        if (available)
            notify(tr("yt-dlp %1 — готов").arg(info));
        else
            notify(tr("yt-dlp недоступен: %1").arg(info), true);
    });
    connect(m_youtube, &YouTubePanel::playRequested, this, [this](const QString &path) {
        addPaths({path}, false);
        const int index = m_playlist->getAllPaths().indexOf(path);
        if (index < 0)
            return;
        m_userTransport = true;
        m_playlist->setCurrentIndex(index);
        m_userTransport = false;
        m_media->play();
    });
    connect(m_youtube, &YouTubePanel::fileAdded, this, [this](const QString &path) {
        if (m_playlist->getAllPaths().contains(path)) {
            notify(tr("Уже в плейлисте: %1").arg(QFileInfo(path).fileName()));
            return;
        }
        addPaths({path}, false);
    });
    connect(m_youtube, &YouTubePanel::filesAdded, this, [this](const QStringList &paths) {
        addPaths(paths, false);
    });
    connect(m_ytdlpPage, &YtDlpPage::applied, this, [this](const QString &binary) {
        m_ytdlp->setBinary(binary);
        m_ytdlp->checkAvailability();
    });
    connect(m_ytdlpPage, &YtDlpPage::checkRequested, this, [this]() { m_ytdlp->checkAvailability(); });

    // --- обзор -------------------------------------------------------------
    connect(m_discover, &DiscoverPage::playRequested, this, [this](const ChartTrack &track) {
        routeChartTrack(track, false);
    });
    connect(m_discover, &DiscoverPage::saveRequested, this, [this](const ChartTrack &track) {
        routeChartTrack(track, true);
    });
    connect(m_discover, &DiscoverPage::previewRequested, this, &MainWindow::playChartPreview);
    connect(m_discover, &DiscoverPage::soundCloudChartRequested, this,
            &MainWindow::requestSoundCloudChart);

    // Чарт SoundCloud читается через yt-dlp: результаты забирает тот, кто запустил.
    connect(m_ytdlp, &YtDlp::searchFinished, this, [this](const QList<YtDlpResult> &results) {
        if (!m_chartSearchPending)
            return;
        m_chartSearchPending = false;
        addSoundCloudChart(results);
    });
    connect(m_ytdlp, &YtDlp::searchFailed, this, [this](const QString &error) {
        if (!m_chartSearchPending)
            return;
        m_chartSearchPending = false;
        m_discover->setShelfStatus(DiscoverPage::soundCloudShelfId(),
                                   tr("Чарт SoundCloud недоступен: %1").arg(error), true);
        m_discover->setJobStatus(tr("Чарт SoundCloud недоступен — из этой сети нужен VPN/прокси"));
    });

    // --- оформление --------------------------------------------------------
    connect(m_appearance, &AppearancePage::framelessToggled, this, &MainWindow::setFrameless);
}

// --- разделы ----------------------------------------------------------------

void MainWindow::onNavChanged(int row)
{
    if (row < 0 || row >= m_pages->count())
        return;
    m_pages->setCurrentIndex(row);
    updateNavIcons();
}

void MainWindow::updateNavIcons()
{
    if (!m_nav)
        return;

    const Icons::Kind kinds[] = {Icons::Kind::Discover, Icons::Kind::Library, Icons::Kind::YouTube, Icons::Kind::Palette,
                                     Icons::Kind::Settings };
    const QColor normal = ThemeUtil::color(QStringLiteral("text-muted"), QColor(0xa0, 0xa6, 0xb4));
    QColor active = ThemeUtil::color(QStringLiteral("accent"), QColor(0xff, 0x55, 0x00));
    // На светлых темах чистый акцент слишком бледный для текста и иконок.
    if (ThemeUtil::isLight())
        active = active.darker(140);

    for (int i = 0; i < m_nav->count() && i < kNavCount; ++i) {
        const bool current = (i == m_nav->currentRow());
        m_nav->item(i)->setIcon(Icons::icon(kinds[i], current ? active : normal, 18));
    }
}

// --- транспорт --------------------------------------------------------------

void MainWindow::onOpenFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Выберите аудиофайлы"), QDir::homePath(),
        tr("Аудио (*.flac *.mp3 *.wav *.ogg *.m4a *.aac *.opus);;Все файлы (*)"));
    if (files.isEmpty())
        return;
    addPaths(files, false);
}

void MainWindow::addPaths(const QStringList &paths, bool fromDrop)
{
    const QStringList existing = m_playlist->getAllPaths();

    QStringList fresh;
    int duplicates = 0;
    for (const QString &path : paths) {
        if (existing.contains(path) || fresh.contains(path)) {
            ++duplicates;
            continue;
        }
        fresh << path;
    }

    if (fresh.isEmpty()) {
        notify(duplicates > 0 ? tr("Всё это уже в плейлисте") : tr("Нечего добавить"));
        return;
    }

    m_playlist->addTracks(fresh);

    QString message = fromDrop ? tr("Добавлено перетаскиванием: %1").arg(fresh.size())
                               : tr("Добавлено: %1").arg(fresh.size());
    if (duplicates > 0)
        message += tr(" (пропущено дублей: %1)").arg(duplicates);
    if (m_playingIndex < 0) {
        m_playlist->setCurrentIndex(0);
        m_media->play();
    }
    notify(message);
}

void MainWindow::onPlayPause()
{
    if (m_media->playbackState() == QMediaPlayer::PlayingState) {
        m_media->pause();
        return;
    }
    if (m_playingIndex < 0 && m_playlist->count() > 0) {
        m_playlist->setCurrentIndex(0);
        loadTrack(0, true);
        return;
    }
    m_media->play();
}

void MainWindow::onStop()
{
    m_media->stop();
    m_playerBar->setPlaying(false);
    m_playerBar->setPosition(0, m_media->duration());
}

void MainWindow::onNext()
{
    if (!m_playlist->hasNext())
        return;
    m_userTransport = true;
    m_playlist->next();
    m_userTransport = false;
    m_media->play();
}

void MainWindow::onPrevious()
{
    if (!m_playlist->hasPrevious())
        return;
    m_userTransport = true;
    m_playlist->previous();
    m_userTransport = false;
    m_media->play();
}

void MainWindow::onTrackActivated(int row)
{
    if (row < 0)
        return;
    m_userTransport = true;
    m_playlist->setCurrentIndex(row);
    m_userTransport = false;
    m_media->play();
}

void MainWindow::onSeek(int positionMs)
{
    m_media->setPosition(positionMs);
}

void MainWindow::onVolumeChanged(int value)
{
    m_audio->setVolume(value / 100.0);
    QSettings().setValue(QStringLiteral("player/volume"), value);
}

void MainWindow::onPositionChanged(qint64 position)
{
    m_playerBar->setPosition(position, m_media->duration());
}

void MainWindow::onDurationChanged(qint64 duration)
{
    m_playerBar->setPosition(m_media->position(), duration);
}

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::EndOfMedia:
        if (m_previewPlaying) {
            // Играло превью из чарта — плейлист не трогаем.
            m_previewPlaying = false;
            m_media->stop();
            m_playerBar->setPlaying(false);
            break;
        }
        // Автопереход: раньше плейлист останавливался на первом же треке.
        if (m_playlist->hasNext()) {
            onNext();
        } else {
            m_playerBar->setPlaying(false);
            m_media->stop();
        }
        break;
    case QMediaPlayer::InvalidMedia:
        notify(tr("Файл не читается: %1").arg(QFileInfo(m_playlist->getTrackPath(m_playingIndex))
                                                  .fileName()),
               true);
        break;
    default:
        break;
    }
}

void MainWindow::onMediaError(QMediaPlayer::Error error, const QString &errorString)
{
    if (error == QMediaPlayer::NoError)
        return;

    QString text = errorString.trimmed();
    if (text.isEmpty())
        text = tr("неизвестная ошибка воспроизведения");
    const QString name = QFileInfo(m_playlist->getTrackPath(m_playingIndex)).fileName();
    notify(name.isEmpty() ? text : tr("%1: %2").arg(name, text), true);
}

// --- модель → представление -------------------------------------------------

void MainWindow::onPlaylistTrackAdded(int index)
{
    const Track track = m_playlist->getTrack(index);
    m_library->addTrack(track.title, track.artist, track.durationText(), track.album);
    refreshTransport();
}

void MainWindow::onPlaylistTrackRemoved(int index)
{
    m_library->removeRow(index);
    if (index == m_playingIndex)
        m_playingIndex = -1;
    refreshTransport();
}

void MainWindow::onCurrentTrackChanged(int index)
{
    m_library->setPlayingRow(index);

    if (index < 0) {
        m_media->stop();
        m_playerBar->clearTrack();
        m_playingIndex = -1;
        refreshTransport();
        return;
    }

    const bool autoplay = m_userTransport
                          || m_media->playbackState() == QMediaPlayer::PlayingState;
    loadTrack(index, autoplay);
}

void MainWindow::loadTrack(int index, bool autoplay)
{
    m_previewPlaying = false;

    const Track track = m_playlist->getTrack(index);
    if (track.filePath.isEmpty())
        return;

    // Если играет тот же файл (например, индекс сдвинулся после удаления трека
    // выше), источник не переставляем — иначе трек начнётся с начала.
    const bool sameSource = m_media->source() == QUrl::fromLocalFile(track.filePath);

    m_playingIndex = index;
    if (!sameSource) {
        m_media->setSource(QUrl::fromLocalFile(track.filePath));
        m_playerBar->setTrack(track.title, track.artist, track.durationMs, track.cover);
    }

    m_playerBar->setControlsEnabled(true);
    m_library->setPlayingRow(index);
    refreshTransport();

    if (autoplay && m_media->playbackState() != QMediaPlayer::PlayingState)
        m_media->play();

    // Акцент из обложки — как «Cover Colors» в SoundCloud Desktop.
    if (!track.cover.isNull() && !sameSource)
        ThemeManager::instance()->applyCoverPalette(track.cover);
}

void MainWindow::refreshTransport()
{
    m_playerBar->setNavigationEnabled(m_playlist->hasPrevious(), m_playlist->hasNext());
    m_playerBar->setControlsEnabled(m_playingIndex >= 0 || m_playlist->count() > 0);
}

// --- обзор (живая подборка) -------------------------------------------------

void MainWindow::routeChartTrack(const ChartTrack &track, bool saveToLibrary)
{
    const QString query = track.searchQuery();
    if (query.isEmpty()) {
        notify(tr("У этой строки чарта нет названия — искать нечего"), true);
        return;
    }

    if (YtDlp::resolveBinary().isEmpty()) {
        notify(tr("yt-dlp не найден — укажите путь в разделе «yt-dlp» слева"), true);
        m_discover->setJobStatus(tr("yt-dlp не найден"));
        return;
    }

    if (m_ytdlp->isBusy()) {
        notify(tr("yt-dlp занят — дождитесь окончания операции"), true);
        return;
    }

    // Строка чарта превращается в обычный запрос на вкладке YouTube: там виден
    // прогресс, а плеер получает готовый файл штатным путём.
    m_youtube->runQuery(query, -1, saveToLibrary);
    m_nav->setCurrentRow(kYouTubeRow);
    notify(saveToLibrary ? tr("Сохраняю в библиотеку: %1").arg(query)
                         : tr("Ищу для прослушивания: %1").arg(query));
}

void MainWindow::playChartPreview(const ChartTrack &track)
{
    if (track.previewUrl.isEmpty()) {
        notify(tr("У этого трека нет превью — слушайте через yt-dlp"), true);
        return;
    }

    m_previewPlaying = true;
    m_media->setSource(QUrl(track.previewUrl));
    m_playerBar->setTrack(track.title, track.artist, track.durationMs, QImage());
    m_playerBar->setControlsEnabled(true);
    m_media->play();
    notify(tr("Превью: %1 — %2").arg(track.artist, track.title));
}

void MainWindow::requestSoundCloudChart(const QString &url)
{
    if (YtDlp::resolveBinary().isEmpty()) {
        m_discover->setShelfStatus(DiscoverPage::soundCloudShelfId(),
                                   tr("yt-dlp не найден — укажите путь в разделе «yt-dlp»"), true);
        return;
    }

    if (m_ytdlp->isBusy()) {
        m_discover->setShelfStatus(DiscoverPage::soundCloudShelfId(),
                                   tr("yt-dlp занят другой задачей — повторите позже"), true);
        return;
    }

    m_chartSearchPending = true;
    m_ytdlp->browse(url, 30);
}

void MainWindow::addSoundCloudChart(const QList<YtDlpResult> &results)
{
    if (results.isEmpty()) {
        m_discover->setShelfStatus(DiscoverPage::soundCloudShelfId(),
                                   tr("Чарт SoundCloud пуст — похоже, сервис недоступен"),
                                   true);
        return;
    }

    const QList<ChartTrack> tracks = chartTracksFromYtDlp(results);

    DiscoverPage::Shelf shelf;
    shelf.id = DiscoverPage::soundCloudShelfId();
    shelf.title = tr("Чарт SoundCloud");
    shelf.subtitle = tr("%1 треков из чарта SoundCloud").arg(tracks.size());
    shelf.tracks = tracks;
    m_discover->addShelf(shelf);
    m_discover->setJobStatus(tr("Чарт SoundCloud: %1 треков").arg(tracks.size()));
}

// --- меню и окно ------------------------------------------------------------

void MainWindow::onShowMenu(const QPoint &globalPos)
{
    QMenu menu(this);
    menu.addAction(tr("Открыть файлы…"), QKeySequence::Open, this, &MainWindow::onOpenFiles);
    menu.addAction(tr("Настройки yt-dlp…"), this, &MainWindow::onYtDlpSettings);
    menu.addSeparator();
    menu.addAction(tr("Открыть папку библиотеки"), this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(YtDlp::libraryDir()));
    });
    menu.addAction(tr("Открыть папку тем"), this, []() {
        ThemeManager::instance()->openThemesFolder();
    });
    menu.addAction(tr("Раздел «Оформление»"), this, [this]() { m_nav->setCurrentRow(kAppearanceRow); });
    menu.addSeparator();
    menu.addAction(tr("Сбросить оформление"), this, [this]() {
        ThemeManager::instance()->resetAppearance();
        notify(tr("Оформление сброшено"));
    });
    menu.addAction(tr("О программе"), this, [this]() {
        QMessageBox::about(this, tr("О программе"),
                           tr("<b>FLACORE</b> — проигрыватель FLAC/MP3 и клиент yt-dlp.<br><br>"
                              "Версия: %1<br>Тем: %2<br>Папка тем: %3")
                               .arg(QCoreApplication::applicationVersion())
                               .arg(ThemeManager::instance()->themes().size())
                               .arg(QDir::toNativeSeparators(ThemeManager::userThemesDir())));
    });
    menu.addSeparator();
    menu.addAction(tr("Выход"), this, [this]() { close(); });
    menu.exec(globalPos);
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

    m_ytdlp->setBinary(dialog.binaryPath());
    m_ytdlp->checkAvailability();
    m_ytdlpPage->form()->setBinaryPath(dialog.binaryPath());
    notify(tr("Настройки yt-dlp сохранены"));
}

void MainWindow::toggleMaximize()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

void MainWindow::setFrameless(bool frameless)
{
    applyFrameless(frameless);
}

void MainWindow::applyFrameless(bool frameless)
{
    m_frameless = frameless;
    const bool visible = isVisible();
    const QRect geometry = this->geometry();

    // Issue #7: WA_TranslucentBackground должен быть установлен до изменения
    // флага окна и до show(), иначе углы не скругляются корректно (особенно
    // при первом включении режима frameless на Windows).
    // Сначала скрываем окно — это необходимо для корректного пересоздания
    // нативного дескриптора окна при изменении флага FramelessWindowHint.
    if (visible)
        hide();

    setAttribute(Qt::WA_TranslucentBackground, frameless);
    setWindowFlag(Qt::FramelessWindowHint, frameless);

    m_titleBar->setFrameless(frameless);
    m_backdrop->setFramelessMode(frameless);

    const bool maximized = isMaximized();
    m_inset = (frameless && !maximized) ? kTopInset : 0;
    const int radius = frameless ? ThemeUtil::number(QStringLiteral("radius"), 16) + 4 : 0;
    m_backdrop->setPanelInset(m_inset);
    m_backdrop->setCornerRadius(maximized ? 0 : radius);

    if (auto *layout = qobject_cast<QVBoxLayout *>(m_backdrop->layout()))
        layout->setContentsMargins(m_inset, m_inset, m_inset, m_inset);

    if (visible) {
        show();
        setGeometry(geometry);
    }
    positionToast();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() != QEvent::WindowStateChange || !m_frameless)
        return;

    const bool maximized = isMaximized();
    m_inset = maximized ? 0 : kTopInset;
    const int radius = ThemeUtil::number(QStringLiteral("radius"), 16) + 4;
    m_backdrop->setPanelInset(m_inset);
    m_backdrop->setCornerRadius(maximized ? 0 : radius);
    if (auto *layout = qobject_cast<QVBoxLayout *>(m_backdrop->layout()))
        layout->setContentsMargins(m_inset, m_inset, m_inset, m_inset);

    m_titleBar->setMaximized(maximized);
    positionToast();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionToast();
}

void MainWindow::positionToast()
{
    if (!m_toast)
        return;

    m_toast->adjustSize();
    const int x = (width() - m_toast->width()) / 2;
    const int y = height() - m_playerBar->height() - m_toast->height() - m_inset - 26;
    m_toast->move(qMax(m_inset + 8, x), qMax(m_inset + 8, y));
}

void MainWindow::notify(const QString &text, bool error)
{
    if (text.isEmpty())
        return;

    m_toast->setProperty("role", error ? QStringLiteral("danger") : QStringLiteral("caption"));
    m_toast->style()->unpolish(m_toast);
    m_toast->style()->polish(m_toast);
    m_toast->setText(text);
    m_toast->show();
    m_toast->raise();
    positionToast();
    m_toastTimer->start();
}
