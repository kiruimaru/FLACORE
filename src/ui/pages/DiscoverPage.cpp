#include "DiscoverPage.h"

#include "../theme/ThemeUtil.h"
#include "../widgets/GlassWidgets.h"
#include "../widgets/Icons.h"
#include "../widgets/ShelfDelegate.h"

#include "core/online/CoverLoader.h"
#include "core/ytdlp/YtDlp.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QtGlobal>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QPushButton *makeButton(const QString &text, const QString &variant, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    if (!variant.isEmpty())
        button->setProperty("variant", variant);
    return button;
}

} // namespace

DiscoverPage::DiscoverPage(QWidget *parent)
    : QWidget(parent)
    , m_covers(new CoverLoader(this))
    , m_fetcher(new ChartsFetcher(this))
{
    buildUi();

    connect(m_covers, &CoverLoader::loaded, this, [this]() {
        for (QListWidget *list : m_lists)
            list->viewport()->update();
    });

    connect(m_fetcher, &ChartsFetcher::deezerReady, this, [this](const QList<ChartTrack> &tracks) {
        fillShelf(deezerShelfId(), tracks,
                  tr("%1 — %2 треков")
                      .arg(m_deezerGenre->currentText(), QString::number(tracks.size())));
    });
    connect(m_fetcher, &ChartsFetcher::appleReady, this, [this](const QList<ChartTrack> &tracks) {
        fillShelf(appleShelfId(), tracks,
                  tr("%1, %2 — %3 треков")
                      .arg(m_appleFeed->currentText(), m_appleCountry->currentText(),
                           QString::number(tracks.size())));
    });
    connect(m_fetcher, &ChartsFetcher::failed, this, [this](const QString &error, bool deezer) {
        // Помечаем только ту полку, чей источник не ответил: вторая может быть цела.
        setShelfStatus(deezer ? deezerShelfId() : appleShelfId(), error, true);
        setJobStatus(error);
    });
}

// --- интерфейс --------------------------------------------------------------

void DiscoverPage::buildUi()
{
    setProperty("dataUi", "main");

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(m_scroll);

    m_content = new QWidget(m_scroll);
    m_content->setProperty("dataUi", "main");
    m_scroll->setWidget(m_content);

    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(18, 16, 18, 24);
    m_contentLayout->setSpacing(14);

    auto *title = new QLabel(tr("Обзор"), m_content);
    title->setProperty("role", "title");

    auto *subtitle = new QLabel(tr("Живые чарты: топ Deezer по жанрам, Apple Music по странам и "
                                   "чарт SoundCloud. Слушать можно превью на 30 секунд или "
                                   "скачать полноценный трек через yt-dlp."),
                                m_content);
    subtitle->setProperty("role", "caption");
    subtitle->setWordWrap(true);

    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);
    auto *refreshAll = makeButton(tr("Обновить всё"), QStringLiteral("accent"), m_content);
    refreshAll->setIcon(Icons::icon(Icons::Kind::Refresh,
                                    ThemeUtil::color(QStringLiteral("accent-contrast"), Qt::white),
                                    16));
    m_jobStatus = new QLabel(m_content);
    m_jobStatus->setProperty("role", "caption");
    m_jobStatus->setWordWrap(true);

    headerRow->addWidget(refreshAll);
    headerRow->addWidget(m_jobStatus, 1);

    m_contentLayout->addWidget(title);
    m_contentLayout->addWidget(subtitle);
    m_contentLayout->addLayout(headerRow);

    // --- полка Deezer -------------------------------------------------------
    auto *deezerControls = new QWidget(m_content);
    auto *deezerLayout = new QHBoxLayout(deezerControls);
    deezerLayout->setContentsMargins(0, 0, 0, 0);
    deezerLayout->setSpacing(8);

    m_deezerGenre = new QComboBox(deezerControls);
    m_deezerGenre->setObjectName(QStringLiteral("deezerGenre"));
    for (const ChartSource &source : deezerGenres())
        m_deezerGenre->addItem(source.name, source.id);

    auto *deezerShow = makeButton(tr("Показать"), QStringLiteral("accent"), deezerControls);
    deezerLayout->addWidget(m_deezerGenre);
    deezerLayout->addWidget(deezerShow);
    deezerLayout->addStretch(1);

    createShelf(deezerShelfId(), tr("Топ Deezer"),
                tr("Чарт по всему миру или по жанру"), deezerControls);

    // --- полка Apple Music --------------------------------------------------
    auto *appleControls = new QWidget(m_content);
    auto *appleLayout = new QHBoxLayout(appleControls);
    appleLayout->setContentsMargins(0, 0, 0, 0);
    appleLayout->setSpacing(8);

    m_appleFeed = new QComboBox(appleControls);
    m_appleFeed->setObjectName(QStringLiteral("appleFeed"));
    for (const ChartSource &source : appleFeeds())
        m_appleFeed->addItem(source.name, source.id);

    m_appleCountry = new QComboBox(appleControls);
    m_appleCountry->setObjectName(QStringLiteral("appleCountry"));
    for (const ChartSource &source : appleCountries())
        m_appleCountry->addItem(source.name, source.id);

    auto *appleShow = makeButton(tr("Показать"), QStringLiteral("accent"), appleControls);
    appleLayout->addWidget(m_appleFeed);
    appleLayout->addWidget(m_appleCountry);
    appleLayout->addWidget(appleShow);
    appleLayout->addStretch(1);

    createShelf(appleShelfId(), tr("Топ Apple Music"),
                tr("Самые прослушиваемые и новинки по стране"), appleControls);

    // --- полка SoundCloud ---------------------------------------------------
    auto *soundCloudControls = new QWidget(m_content);
    auto *soundCloudLayout = new QHBoxLayout(soundCloudControls);
    soundCloudLayout->setContentsMargins(0, 0, 0, 0);
    soundCloudLayout->setSpacing(8);

    m_soundCloudGenre = new QComboBox(soundCloudControls);
    m_soundCloudGenre->setObjectName(QStringLiteral("soundCloudGenre"));
    for (const ChartSource &source : soundCloudGenres())
        m_soundCloudGenre->addItem(source.name, source.id);

    auto *soundCloudShow = makeButton(tr("Показать"), QStringLiteral("accent"), soundCloudControls);
    soundCloudLayout->addWidget(m_soundCloudGenre);
    soundCloudLayout->addWidget(soundCloudShow);
    soundCloudLayout->addStretch(1);

    createShelf(soundCloudShelfId(), tr("Чарт SoundCloud"),
                tr("Идёт через yt-dlp: если SoundCloud недоступен из вашей сети, "
                   "полка останется пустой — нужен VPN/прокси"),
                soundCloudControls);

    m_contentLayout->addStretch(1);

    // --- связи --------------------------------------------------------------
    connect(refreshAll, &QPushButton::clicked, this, &DiscoverPage::refreshAll);
    connect(deezerShow, &QPushButton::clicked, this, [this]() {
        m_fetcher->fetchDeezer(m_deezerGenre->currentData().toString(), 30);
    });
    connect(appleShow, &QPushButton::clicked, this, [this]() {
        m_fetcher->fetchApple(m_appleFeed->currentData().toString(),
                              m_appleCountry->currentData().toString(), 30);
    });
    connect(soundCloudShow, &QPushButton::clicked, this,
            &DiscoverPage::requestSoundCloudChart);

    connect(m_deezerGenre, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            m_fetcher->fetchDeezer(m_deezerGenre->currentData().toString(), 30);
    });
    connect(m_appleCountry, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            m_fetcher->fetchApple(m_appleFeed->currentData().toString(),
                                  m_appleCountry->currentData().toString(), 30);
    });
    connect(m_soundCloudGenre, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            requestSoundCloudChart();
    });
}

QListWidget *DiscoverPage::createShelf(const QString &id, const QString &title,
                                       const QString &subtitle, QWidget *controls)
{
    auto *card = new GlassPanel(m_content);
    card->setHook(QStringLiteral("card"));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    auto *header = new QHBoxLayout();
    header->setSpacing(10);

    auto *titles = new QVBoxLayout();
    titles->setSpacing(0);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("role", "section");
    titles->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setProperty("role", "faint");
    subtitleLabel->setWordWrap(true);
    titles->addWidget(subtitleLabel);

    header->addLayout(titles, 1);
    if (controls) {
        controls->setParent(card);
        header->addWidget(controls);
    }
    layout->addLayout(header);

    auto *list = new QListWidget(card);
    list->setObjectName(QStringLiteral("shelf_") + id);
    list->setProperty("dataUi", "results");
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setItemDelegate(new ShelfDelegate(m_covers, list));
    list->setMinimumHeight(0);
    // Пустая полка показывает подсказку, а не пустую карточку.
    setShelfHint(list, tr("Пока пусто — нажмите «Обновить всё»"));
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(list);

    auto *status = new QLabel(card);
    status->setProperty("role", "caption");
    status->setWordWrap(true);
    layout->addWidget(status);

    layout->addStretch(0);

    m_lists.insert(id, list);
    m_statuses.insert(id, status);
    m_contentLayout->addWidget(card);

    connect(list, &QListWidget::itemDoubleClicked, this, [this, list](QListWidgetItem *item) {
        // Берём именно тот элемент, по которому кликнули: при двойном клике
        // currentRow() ещё может быть пустым.
        const ChartTrack track = trackAt(list, list->row(item));
        if (!track.title.isEmpty())
            emit playRequested(track);
    });
    connect(list, &QListWidget::customContextMenuRequested, this, [this, list](const QPoint &point) {
        openContextMenuFor(list, point);
    });

    return list;
}

// --- данные -----------------------------------------------------------------

void DiscoverPage::addShelf(const Shelf &shelf)
{
    QListWidget *list = m_lists.value(shelf.id);
    if (!list) {
        list = createShelf(shelf.id, shelf.title, shelf.subtitle, nullptr);
    }

    if (shelf.tracks.isEmpty()) {
        setShelfHint(list, tr("Пусто — нажмите «Обновить всё»"));
        setShelfStatus(shelf.id, shelf.subtitle);
        return;
    }

    list->clear();
    for (const ChartTrack &track : shelf.tracks) {
        auto *item = new QListWidgetItem(track.title, list);
        item->setData(ShelfDelegate::ArtistRole, track.artist);
        item->setData(ShelfDelegate::DurationRole, track.durationText());
        item->setData(ShelfDelegate::CoverUrlRole, track.coverUrl);
        item->setData(ShelfDelegate::SourceRole, track.sourceName());
        item->setData(ShelfDelegate::UrlRole, track.url);
        item->setData(ShelfDelegate::PreviewUrlRole, track.previewUrl);
        item->setToolTip(QStringLiteral("%1 — %2\n%3\n%4")
                             .arg(track.artist, track.title, track.album, track.url));
    }

    list->setMaximumHeight(qMin(6, shelf.tracks.size()) * ShelfDelegate::RowHeight + 8);
    setShelfStatus(shelf.id, shelf.subtitle);
}

void DiscoverPage::fillShelf(const QString &id, const QList<ChartTrack> &tracks,
                             const QString &subtitle)
{
    QListWidget *list = m_lists.value(id);
    if (!list)
        return;

    if (tracks.isEmpty()) {
        setShelfHint(list, tr("Ничего не пришло — проверьте подключение"));
        setShelfStatus(id, subtitle);
        return;
    }

    list->clear();
    for (const ChartTrack &track : tracks) {
        auto *item = new QListWidgetItem(track.title, list);
        item->setData(ShelfDelegate::ArtistRole, track.artist);
        item->setData(ShelfDelegate::DurationRole, track.durationText());
        item->setData(ShelfDelegate::CoverUrlRole, track.coverUrl);
        item->setData(ShelfDelegate::SourceRole, track.sourceName());
        item->setData(ShelfDelegate::UrlRole, track.url);
        item->setData(ShelfDelegate::PreviewUrlRole, track.previewUrl);
        item->setToolTip(QStringLiteral("%1 — %2\n%3").arg(track.artist, track.title, track.album));

        // Обложку тянем сразу: делегат покажет её, как только она придёт.
        if (!track.coverUrl.isEmpty())
            m_covers->request(track.coverUrl);
    }

    list->setMaximumHeight(qMin(6, tracks.size()) * ShelfDelegate::RowHeight + 8);
    setShelfStatus(id, subtitle);
}

int DiscoverPage::shelfCount() const
{
    return m_lists.size();
}

QListWidget *DiscoverPage::shelfList(const QString &id) const
{
    return m_lists.value(id);
}

int DiscoverPage::shelfTrackCount(const QString &id) const
{
    QListWidget *list = m_lists.value(id);
    if (!list)
        return 0;

    int rows = 0;
    for (int i = 0; i < list->count(); ++i) {
        const QListWidgetItem *item = list->item(i);
        if (!item->data(ShelfDelegate::HintRole).toBool() && !item->text().isEmpty())
            ++rows;
    }
    return rows;
}

void DiscoverPage::setShelfHint(QListWidget *list, const QString &text)
{
    list->clear();
    if (text.isEmpty()) {
        list->setMaximumHeight(0);
        return;
    }

    auto *item = new QListWidgetItem(text, list);
    item->setFlags(Qt::NoItemFlags);      // подсказку нельзя выбрать
    item->setData(ShelfDelegate::HintRole, true);
    item->setTextAlignment(Qt::AlignCenter);
    list->setMaximumHeight(ShelfDelegate::RowHeight + 8);
}

void DiscoverPage::setShelfStatus(const QString &id, const QString &text, bool error)
{
    QLabel *label = m_statuses.value(id);
    if (!label)
        return;

    label->setProperty("role", error ? QStringLiteral("danger") : QStringLiteral("caption"));
    label->style()->unpolish(label);
    label->style()->polish(label);
    label->setText(text);
}

void DiscoverPage::setJobStatus(const QString &text)
{
    m_jobStatus->setText(text);
}

ChartTrack DiscoverPage::trackAt(QListWidget *list, int row) const
{
    ChartTrack track;
    if (!list || row < 0 || row >= list->count())
        return track;

    const QListWidgetItem *item = list->item(row);
    track.title = item->text();
    track.artist = item->data(ShelfDelegate::ArtistRole).toString();
    track.coverUrl = item->data(ShelfDelegate::CoverUrlRole).toString();
    track.source = item->data(ShelfDelegate::SourceRole).toString();
    track.url = item->data(ShelfDelegate::UrlRole).toString();
    track.previewUrl = item->data(ShelfDelegate::PreviewUrlRole).toString();
    return track;
}

void DiscoverPage::openContextMenuFor(QListWidget *list, const QPoint &position)
{
    const int row = list->indexAt(position).row();
    if (row < 0)
        return;
    list->setCurrentRow(row);

    const ChartTrack track = trackAt(list, row);
    if (track.title.isEmpty())
        return;

    QMenu menu(this);
    if (!track.previewUrl.isEmpty())
        menu.addAction(tr("▶  Превью 30 секунд"), this, [this, track]() {
            emit previewRequested(track);
        });
    menu.addAction(tr("Слушать полностью (через yt-dlp)"), this, [this, track]() {
        emit playRequested(track);
    });
    menu.addAction(tr("Сохранить в библиотеку"), this, [this, track]() {
        emit saveRequested(track);
    });
    menu.addSeparator();
    if (!track.url.isEmpty())
        menu.addAction(tr("Открыть страницу трека"), this, [track]() {
            QDesktopServices::openUrl(QUrl(track.url));
        });
    menu.addAction(tr("Скопировать «исполнитель — название»"), this, [track]() {
        QApplication::clipboard()->setText(track.searchQuery());
    });
    menu.exec(list->viewport()->mapToGlobal(position));
}

void DiscoverPage::refreshAll()
{
    m_jobStatus->setText(tr("Загружаю чарты…"));
    m_fetcher->fetchDeezer(m_deezerGenre->currentData().toString(), 30);
    m_fetcher->fetchApple(m_appleFeed->currentData().toString(),
                          m_appleCountry->currentData().toString(), 30);
    requestSoundCloudChart();
}

void DiscoverPage::requestSoundCloudChart()
{
    setShelfStatus(soundCloudShelfId(), tr("Читаю чарт SoundCloud…"));
    setJobStatus(tr("Читаю чарт SoundCloud через yt-dlp…"));
    emit soundCloudChartRequested(soundCloudChartUrl(m_soundCloudGenre->currentData().toString()));
}

void DiscoverPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_loadedOnce)
        return;

    m_loadedOnce = true;

    // FLACORE_OFFLINE=1 — не ходить в сеть вообще (автотесты, работа без интернета).
    if (qEnvironmentVariableIsSet("FLACORE_OFFLINE"))
        return;

    // Первая загрузка — только когда страницу реально открыли.
    m_fetcher->fetchDeezer(m_deezerGenre->currentData().toString(), 30);
    m_fetcher->fetchApple(m_appleFeed->currentData().toString(),
                          m_appleCountry->currentData().toString(), 30);
}