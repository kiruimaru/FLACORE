#include "YouTubePanel.h"

#include "core/ytdlp/YtDlp.h"

#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kUrlRole = Qt::UserRole;
constexpr int kTitleRole = Qt::UserRole + 1;
} // namespace

YouTubePanel::YouTubePanel(YtDlp *ytdlp, QWidget *parent)
    : QWidget(parent)
    , m_ytdlp(ytdlp)
{
    setupUi();

    connect(m_ytdlp, &YtDlp::searchFinished, this, [this](const QList<YtDlpResult> &results) {
        m_results->clear();
        for (const YtDlpResult &result : results)
            addResultItem(result);
        m_results->setCurrentRow(0);
        setStatus(tr("Найдено: %1").arg(results.size()));
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::searchFailed, this, [this](const QString &error) {
        m_results->clear();
        setError(error);
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::downloadProgress, this,
            [this](int percent, const QString &speed, const QString &eta, qint64, qint64) {
                if (percent >= 0) {
                    m_progress->setRange(0, 100);
                    m_progress->setValue(percent);
                }
                QStringList parts;
                parts << tr("%1%").arg(percent >= 0 ? QString::number(percent) : QStringLiteral("…"));
                if (!speed.isEmpty())
                    parts << speed;
                if (!eta.isEmpty())
                    parts << tr("осталось %1").arg(eta);
                setStatus(parts.join(QStringLiteral(" · ")));
            });

    connect(m_ytdlp, &YtDlp::downloadStatus, this, [this](const QString &status) {
        if (status.startsWith(QLatin1String("[download]")) && status.contains(QLatin1Char('%')))
            return; // дублирует прогресс
        setStatus(status);
    });

    connect(m_ytdlp, &YtDlp::downloadFinished, this, [this](const QString &filePath) {
        const Job job = m_job;
        m_job = Job::None;
        m_progress->setRange(0, 100);
        m_progress->setValue(100);

        if (!QFileInfo::exists(filePath)) {
            setError(tr("Файл не найден после скачивания: %1").arg(filePath));
            updateUi();
            return;
        }

        const QString name = QFileInfo(filePath).completeBaseName();
        if (job == Job::SaveToLibrary) {
            setStatus(tr("Сохранено: %1").arg(name));
            emit fileAdded(filePath);
        } else {
            setStatus(tr("Играю: %1").arg(name));
            emit playRequested(filePath);
        }
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::downloadFailed, this, [this](const QString &error) {
        m_job = Job::None;
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        setError(error);
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::cancelled, this, [this]() {
        m_job = Job::None;
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        setStatus(tr("Отменено"));
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::availabilityChecked, this,
            [this](bool available, const QString &info) {
                // Проверка версии делает yt-dlp «занятым»: после неё нужно
                // разблокировать кнопки, иначе панель остаётся серой.
                if (available)
                    setStatus(tr("yt-dlp %1 — готов. Введите запрос или вставьте ссылку.").arg(info));
                else
                    setError(tr("yt-dlp недоступен: %1").arg(info));
                updateUi();
            });

    updateUi();
}

void YouTubePanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    auto *searchRow = new QHBoxLayout();
    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->setObjectName(QStringLiteral("ytSourceCombo"));
    m_sourceCombo->addItem(tr("YouTube"), int(YtDlp::Provider::YouTube));
    m_sourceCombo->addItem(tr("SoundCloud"), int(YtDlp::Provider::SoundCloud));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("ytSearchEdit"));
    m_searchEdit->setPlaceholderText(tr("Поиск или ссылка (YouTube, SoundCloud, …)"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchButton = new QPushButton(tr("Найти"), this);
    m_searchButton->setObjectName(QStringLiteral("ytSearchButton"));
    m_searchButton->setDefault(true);
    searchRow->addWidget(m_sourceCombo);
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_searchButton);
    layout->addLayout(searchRow);

    m_results = new QListWidget(this);
    m_results->setObjectName(QStringLiteral("ytResults"));
    m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_results->setAlternatingRowColors(true);
    m_results->setWordWrap(true);
    layout->addWidget(m_results, 1);

    auto *actionsRow = new QHBoxLayout();
    m_playButton = new QPushButton(tr("Слушать"), this);
    m_playButton->setObjectName(QStringLiteral("ytPlayButton"));
    m_playButton->setToolTip(tr("Скачать во временный кэш и сразу включить"));
    m_saveButton = new QPushButton(tr("В библиотеку"), this);
    m_saveButton->setObjectName(QStringLiteral("ytSaveButton"));
    m_saveButton->setToolTip(tr("Скачать в %1").arg(YtDlp::libraryDir()));
    m_cancelButton = new QPushButton(tr("Отмена"), this);
    m_cancelButton->setObjectName(QStringLiteral("ytCancelButton"));
    m_cancelButton->setEnabled(false);
    actionsRow->addWidget(m_playButton);
    actionsRow->addWidget(m_saveButton);
    actionsRow->addStretch();
    actionsRow->addWidget(m_cancelButton);
    layout->addLayout(actionsRow);

    m_progress = new QProgressBar(this);
    m_progress->setObjectName(QStringLiteral("ytProgress"));
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    layout->addWidget(m_progress);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("ytStatus"));
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color: palette(mid);"));
    layout->addWidget(m_status);

    connect(m_searchButton, &QPushButton::clicked, this, &YouTubePanel::runSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &YouTubePanel::runSearch);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() { updateUi(); });
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this, [this]() { updateUi(); });
    connect(m_playButton, &QPushButton::clicked, this, [this]() { startJob(Job::PlayNow); });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() { startJob(Job::SaveToLibrary); });
    connect(m_cancelButton, &QPushButton::clicked, m_ytdlp, &YtDlp::cancel);
    connect(m_results, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { startJob(Job::PlayNow); });
    connect(m_results, &QListWidget::itemSelectionChanged, this, [this]() { updateUi(); });

    setStatus(YtDlp::resolveBinary().isEmpty()
                  ? tr("yt-dlp не найден — задайте путь в «Инструменты → Настройки yt-dlp».")
                  : tr("Введите запрос или вставьте ссылку."));
}

void YouTubePanel::runSearch()
{
    const QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty())
        return;

    m_results->clear();
    m_progress->setRange(0, 0); // «занято»
    setStatus(tr("Ищу: %1").arg(query));
    updateUi();

    const auto provider = static_cast<YtDlp::Provider>(
        m_sourceCombo->currentData().toInt());
    m_ytdlp->search(query, 15, provider);
}

void YouTubePanel::startJob(Job job)
{
    QString url;
    QString title;
    if (!selectedTarget(&url, &title))
        return;

    if (YtDlp::resolveBinary().isEmpty()) {
        setError(tr("yt-dlp не найден. Укажите путь в «Инструменты → Настройки yt-dlp»."));
        return;
    }

    m_job = job;
    m_progress->setRange(0, 0);
    m_progress->setValue(0);

    if (job == Job::SaveToLibrary) {
        const QString format = QSettings().value(QStringLiteral("ytdlp/audioFormat"),
                                                 QStringLiteral("flac")).toString();
        setStatus(tr("Скачиваю в библиотеку (%1): %2").arg(format, title));
        m_ytdlp->download(url, YtDlp::libraryDir(), format);
    } else {
        setStatus(tr("Загружаю для прослушивания: %1").arg(title));
        m_ytdlp->download(url, YtDlp::cacheDir(), QString());
    }

    updateUi();
}

void YouTubePanel::updateUi()
{
    const bool busy = m_ytdlp->isBusy();
    const bool hasSelection = m_results->currentItem() != nullptr;

    m_searchEdit->setEnabled(!busy);
    m_searchButton->setEnabled(!busy && !m_searchEdit->text().trimmed().isEmpty());
    m_results->setEnabled(!busy);
    m_playButton->setEnabled(!busy && hasSelection);
    m_saveButton->setEnabled(!busy && hasSelection);
    m_cancelButton->setEnabled(busy && m_ytdlp->mode() == YtDlp::Mode::Downloading);

    if (!busy && m_progress->maximum() == 0)
        m_progress->setRange(0, 100);
}

void YouTubePanel::setStatus(const QString &text)
{
    m_status->setText(text);
    m_status->setStyleSheet(QStringLiteral("color: palette(mid);"));
}

void YouTubePanel::setError(const QString &text)
{
    m_status->setText(text);
    m_status->setStyleSheet(QStringLiteral("color: #ff6b6b;"));
}

bool YouTubePanel::selectedTarget(QString *url, QString *title) const
{
    const QListWidgetItem *item = m_results->currentItem();
    if (!item)
        return false;

    if (url)
        *url = item->data(kUrlRole).toString();
    if (title)
        *title = item->data(kTitleRole).toString();
    return !(url && url->isEmpty());
}

void YouTubePanel::addResultItem(const YtDlpResult &result)
{
    auto *item = new QListWidgetItem(m_results);
    QString label = result.title;
    if (!result.uploader.isEmpty())
        label += QStringLiteral(" — ") + result.uploader;
    label += QStringLiteral("  [") + formatDuration(result.durationSec) + QLatin1Char(']');
    item->setText(label);
    item->setData(kUrlRole, result.webpageUrl);
    item->setData(kTitleRole, result.title);
    item->setToolTip(result.webpageUrl);

    if (!result.durationSec)
        item->setForeground(QColor(0xa0, 0xa0, 0xa0)); // live/неизвестная длительность
}
