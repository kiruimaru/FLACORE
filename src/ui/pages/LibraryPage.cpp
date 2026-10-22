#include "LibraryPage.h"

#include "../theme/ThemeUtil.h"
#include "../widgets/GlassWidgets.h"
#include "../widgets/Icons.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QStringList audioPathsFrom(const QMimeData *mime)
{
    QStringList paths;
    if (!mime || !mime->hasUrls())
        return paths;

    const QStringList supported = {QStringLiteral("flac"), QStringLiteral("mp3"),
                                   QStringLiteral("wav"), QStringLiteral("ogg"),
                                   QStringLiteral("m4a"), QStringLiteral("aac"),
                                   QStringLiteral("opus")};
    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        const QString suffix = path.section(QLatin1Char('.'), -1).toLower();
        if (supported.contains(suffix))
            paths << path;
    }
    return paths;
}

} // namespace

// --- TrackListWidget --------------------------------------------------------

TrackListWidget::TrackListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setAcceptDrops(true);
    setProperty("dataUi", "trackList");
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void TrackListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!audioPathsFrom(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void TrackListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (!audioPathsFrom(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void TrackListWidget::dropEvent(QDropEvent *event)
{
    const QStringList paths = audioPathsFrom(event->mimeData());
    if (paths.isEmpty()) {
        QListWidget::dropEvent(event);
        return;
    }
    emit filesDropped(paths);
    event->acceptProposedAction();
}

// --- LibraryPage ------------------------------------------------------------

LibraryPage::LibraryPage(QWidget *parent)
    : QWidget(parent)
{
    setProperty("dataUi", "main");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 12);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout();
    header->setSpacing(10);

    auto *titles = new QVBoxLayout();
    titles->setSpacing(0);

    auto *title = new QLabel(tr("Библиотека"), this);
    title->setProperty("role", "title");
    titles->addWidget(title);

    m_count = new QLabel(tr("Пусто"), this);
    m_count->setProperty("role", "caption");
    titles->addWidget(m_count);

    header->addLayout(titles);
    header->addStretch(1);

    auto *openButton = new QPushButton(tr("Открыть файлы"), this);
    openButton->setProperty("variant", "accent");
    openButton->setIcon(Icons::icon(Icons::Kind::Plus,
                                    ThemeUtil::color(QStringLiteral("accent-contrast"), Qt::white),
                                    16));
    header->addWidget(openButton);

    auto *removeButton = new QPushButton(tr("Убрать"), this);
    removeButton->setIcon(Icons::icon(Icons::Kind::Trash,
                                      ThemeUtil::color(QStringLiteral("text-muted"), Qt::gray), 16));
    header->addWidget(removeButton);

    auto *clearButton = new QPushButton(tr("Очистить"), this);
    clearButton->setProperty("variant", "danger");
    header->addWidget(clearButton);

    layout->addLayout(header);

    m_list = new TrackListWidget(this);
    m_delegate = new TrackDelegate(m_list);
    m_list->setItemDelegate(m_delegate);

    // Пустое состояние живёт в той же ячейке сетки, что и список, и не ловит
    // мышь: перетаскивание файлов должно доходить до списка.
    m_empty = new QLabel(this);
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setProperty("role", "caption");
    m_empty->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_empty->setText(tr("Тут пока пусто.\n\n"
                        "Перетащите сюда FLAC/MP3/WAV, нажмите «Открыть файлы»\n"
                        "или скачайте что-нибудь на вкладке «YouTube»."));

    auto *stack = new QGridLayout();
    stack->setContentsMargins(0, 0, 0, 0);
    stack->addWidget(m_list, 0, 0);
    stack->addWidget(m_empty, 0, 0, Qt::AlignCenter);
    layout->addLayout(stack, 1);

    connect(openButton, &QPushButton::clicked, this, &LibraryPage::openFilesRequested);
    connect(clearButton, &QPushButton::clicked, this, &LibraryPage::clearRequested);
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeRequested(m_list->currentRow());
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        emit trackActivated(m_list->row(item));
    });
    connect(m_list, &TrackListWidget::filesDropped, this, &LibraryPage::filesDropped);

    refreshEmptyState();
}

void LibraryPage::addTrack(const QString &title, const QString &artist, const QString &durationText,
                           const QString &album)
{
    auto *item = new QListWidgetItem(title, m_list);
    item->setData(TrackDelegate::ArtistRole, artist);
    item->setData(TrackDelegate::DurationRole, durationText);
    item->setData(TrackDelegate::AlbumRole, album);
    item->setToolTip(QStringLiteral("%1\n%2").arg(title, artist));
    refreshEmptyState();
}

void LibraryPage::removeRow(int row)
{
    if (row < 0 || row >= m_list->count())
        return;
    delete m_list->takeItem(row);
    refreshEmptyState();
}

void LibraryPage::clearRows()
{
    m_list->clear();
    m_delegate->setPlayingRow(-1);
    refreshEmptyState();
}

void LibraryPage::setPlayingRow(int row)
{
    m_delegate->setPlayingRow(row);
}

void LibraryPage::setFilter(const QString &text)
{
    const QString needle = text.trimmed();
    int visible = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        const bool matches = needle.isEmpty()
                             || item->text().contains(needle, Qt::CaseInsensitive)
                             || item->data(TrackDelegate::ArtistRole)
                                    .toString()
                                    .contains(needle, Qt::CaseInsensitive);
        item->setHidden(!matches);
        if (matches)
            ++visible;
    }
    updateCountLabel(visible, m_list->count());
}

void LibraryPage::updateCountLabel(int visible, int total)
{
    if (total == 0) {
        m_count->setText(tr("Пусто"));
        return;
    }
    if (visible == total)
        m_count->setText(tr("Треков: %1").arg(total));
    else
        m_count->setText(tr("Найдено %1 из %2").arg(visible).arg(total));
}

void LibraryPage::refreshEmptyState()
{
    const bool empty = m_list->count() == 0;
    m_empty->setVisible(empty);
    if (empty)
        m_count->setText(tr("Пусто"));
    else
        updateCountLabel(m_list->count(), m_list->count());
}