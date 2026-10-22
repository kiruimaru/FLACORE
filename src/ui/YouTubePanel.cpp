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
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kUrlRole = Qt::UserRole;
constexpr int kTitleRole = Qt::UserRole + 1;

/// Сколько найденных вариантов пробовать, если скачивание срывается.
constexpr int kMaxAutoAttempts = 3;
} // namespace

YouTubePanel::YouTubePanel(YtDlp *ytdlp, QWidget *parent)
    : QWidget(parent)
    , m_ytdlp(ytdlp)
{
    setupUi();

    connect(m_ytdlp, &YtDlp::searchFinished, this, [this](const QList<YtDlpResult> &results) {
        if (!m_searchPending)
            return;                  // результаты чужой операции (например, чарта «Обзора»)
        m_searchPending = false;

        m_results->clear();
        for (const YtDlpResult &result : results)
            addResultItem(result);
        m_results->setCurrentRow(0);

        const Job pending = m_pendingJob;
        if (pending != Job::None) {
            if (results.isEmpty()) {
                if (tryFallbackProvider())
                    return;
                m_pendingJob = Job::None;
                showResultsHint(tr("Ничего не найдено по запросу «%1».")
                                    .arg(m_searchEdit->text()));
                setError(tr("Ничего не найдено: %1").arg(m_searchEdit->text()));
                updateUi();
                return;
            }
            m_pendingJob = Job::None;
            startJob(pending);       // m_job выставит сам startJob
            return;
        }

        setStatus(tr("Найдено: %1").arg(results.size()));
        rememberProvider();
        updateUi();
    });

    connect(m_ytdlp, &YtDlp::searchFailed, this, [this](const QString &error) {
        if (!m_searchPending)
            return;
        m_searchPending = false;

        if (m_pendingJob != Job::None && tryFallbackProvider())
            return;

        m_pendingJob = Job::None;
        showResultsHint(tr("Поиск не удался. Проверьте подключение и путь к yt-dlp "
                           "в разделе «yt-dlp»."));
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
                parts << tr("%1%").arg(percent >= 0 ? QString::number(percent)
                                                    : QStringLiteral("…"));
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

        m_autoJob = false;                 // подбор вариантов закончен
        m_autoJobKind = Job::None;
        m_autoAttempts = 0;
        rememberProvider();                // источник сработал — начнём с него

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

        // Первый попавшийся трек может быть DRM-защищён или удалён: из «Обзора»
        // задание запускается автоматически, поэтому имеет смысл взять следующую
        // находку, а не показывать ошибку сразу.
        if (m_autoJob && m_autoAttempts + 1 < kMaxAutoAttempts && selectNextResult()) {
            ++m_autoAttempts;
            setStatus(tr("Не вышло (%1). Пробую следующий вариант — %2 из %3")
                          .arg(error.left(60), QString::number(m_autoAttempts + 1),
                               QString::number(kMaxAutoAttempts)));
            startJob(m_autoJobKind);
            return;
        }

        m_autoJob = false;
        m_autoJobKind = Job::None;
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
                    setStatus(tr("yt-dlp %1 — готов. Введите запрос или вставьте ссылку.")
                                  .arg(info));
                else
                    setError(tr("yt-dlp недоступен: %1").arg(info));
                updateUi();
            });

    updateUi();
}

void YouTubePanel::setupUi()
{
    setProperty("dataUi", "main");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 12);
    layout->setSpacing(10);

    auto *title = new QLabel(tr("YouTube и SoundCloud"), this);
    title->setProperty("role", "title");

    auto *subtitle = new QLabel(tr("Поиск идёт через yt-dlp: «Слушать» качает во временный кэш "
                                   "и сразу включает, «В библиотеку» сохраняет в выбранном "
                                   "формате."),
                                this);
    subtitle->setProperty("role", "caption");
    subtitle->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto *searchRow = new QHBoxLayout();
    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->setObjectName(QStringLiteral("ytSourceCombo"));
    m_sourceCombo->addItem(tr("YouTube"), int(YtDlp::Provider::YouTube));
    m_sourceCombo->addItem(tr("SoundCloud"), int(YtDlp::Provider::SoundCloud));

    // Вспоминаем источник, который в прошлый раз сработал: иначе из сети, где
    // YouTube недоступен, каждый поиск начинается с таймаута.
    const int saved = QSettings().value(QStringLiteral("ytdlp/provider"), -1).toInt();
    if (saved >= 0) {
        const int index = m_sourceCombo->findData(saved);
        if (index >= 0)
            m_sourceCombo->setCurrentIndex(index);
    }

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("ytSearchEdit"));
    m_searchEdit->setProperty("dataUi", "search");
    m_searchEdit->setPlaceholderText(tr("Поиск или ссылка (YouTube, SoundCloud, …)"));
    m_searchEdit->setClearButtonEnabled(true);

    m_searchButton = new QPushButton(tr("Найти"), this);
    m_searchButton->setObjectName(QStringLiteral("ytSearchButton"));
    m_searchButton->setProperty("variant", "accent");
    m_searchButton->setDefault(true);

    searchRow->addWidget(m_sourceCombo);
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_searchButton);
    layout->addLayout(searchRow);

    m_results = new QListWidget(this);
    m_results->setObjectName(QStringLiteral("ytResults"));
    m_results->setProperty("dataUi", "results");
    m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_results->setWordWrap(true);
    layout->addWidget(m_results, 1);

    auto *actionsRow = new QHBoxLayout();
    m_playButton = new QPushButton(tr("Слушать"), this);
    m_playButton->setObjectName(QStringLiteral("ytPlayButton"));
    m_playButton->setProperty("variant", "accent");
    m_playButton->setToolTip(tr("Скачать во временный кэш и сразу включить"));

    m_saveButton = new QPushButton(tr("В библиотеку"), this);
    m_saveButton->setObjectName(QStringLiteral("ytSaveButton"));
    m_saveButton->setToolTip(tr("Скачать в %1").arg(YtDlp::libraryDir()));

    m_cancelButton = new QPushButton(tr("Отмена"), this);
    m_cancelButton->setObjectName(QStringLiteral("ytCancelButton"));
    m_cancelButton->setProperty("variant", "danger");
    m_cancelButton->setVisible(false);   // нужна только во время скачивания

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
    m_status->setProperty("role", "caption");
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    connect(m_searchButton, &QPushButton::clicked, this, [this]() {
        m_pendingJob = Job::None;        // обычный поиск: ничего не запускаем автоматически
        m_fallbackTried = false;
        runSearch();
    });
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        m_pendingJob = Job::None;
        m_fallbackTried = false;
        runSearch();
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() { updateUi(); });
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this, [this]() { updateUi(); });
    connect(m_playButton, &QPushButton::clicked, this, [this]() {
        m_autoJob = false;                 // выбрал пользователь — перебирать не нужно
        startJob(Job::PlayNow);
    });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        m_autoJob = false;
        startJob(Job::SaveToLibrary);
    });
    connect(m_cancelButton, &QPushButton::clicked, m_ytdlp, &YtDlp::cancel);
    connect(m_results, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        m_autoJob = false;
        startJob(Job::PlayNow);
    });
    connect(m_results, &QListWidget::itemSelectionChanged, this, [this]() { updateUi(); });

    setStatus(YtDlp::resolveBinary().isEmpty()
                  ? tr("yt-dlp не найден — задайте путь в разделе «yt-dlp» слева.")
                  : tr("Введите запрос или вставьте ссылку."));
    showResultsHint(tr("Здесь появятся результаты поиска.\n"
                       "Введите запрос (например «daft punk») или вставьте ссылку на трек."));
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

    m_searchPending = true;
    const auto provider = static_cast<YtDlp::Provider>(m_sourceCombo->currentData().toInt());
    m_ytdlp->search(query, 15, provider);
}

void YouTubePanel::runQuery(const QString &query, int provider, bool saveToLibrary)
{
    const QString text = query.trimmed();
    if (text.isEmpty() || m_ytdlp->isBusy())
        return;

    if (provider >= 0) {
        const int index = m_sourceCombo->findData(provider);
        if (index >= 0)
            m_sourceCombo->setCurrentIndex(index);
    }

    m_searchEdit->setText(text);
    m_pendingJob = saveToLibrary ? Job::SaveToLibrary : Job::PlayNow;
    m_autoJob = true;
    m_autoJobKind = m_pendingJob;
    m_autoAttempts = 0;
    m_fallbackTried = false;
    m_job = Job::None;
    runSearch();
}

bool YouTubePanel::selectNextResult()
{
    const int current = m_results->currentRow();
    for (int row = current + 1; row < m_results->count(); ++row) {
        const QListWidgetItem *item = m_results->item(row);
        if (!item || !(item->flags() & Qt::ItemIsEnabled))
            continue;
        if (item->data(kUrlRole).toString().isEmpty())
            continue;

        m_results->setCurrentRow(row);
        return true;
    }
    return false;
}

void YouTubePanel::rememberProvider()
{
    const int provider = m_sourceCombo->currentData().toInt();
    if (provider < 0)
        return;

    QSettings().setValue(QStringLiteral("ytdlp/provider"), provider);
}

bool YouTubePanel::tryFallbackProvider()
{
    if (m_fallbackTried)
        return false;

    const int current = m_sourceCombo->currentData().toInt();
    const int other = current == int(YtDlp::Provider::SoundCloud)
                          ? int(YtDlp::Provider::YouTube)
                          : int(YtDlp::Provider::SoundCloud);
    const int index = m_sourceCombo->findData(other);
    if (index < 0)
        return false;

    m_fallbackTried = true;
    m_sourceCombo->setCurrentIndex(index);
    m_searchPending = true;

    setStatus(tr("В %1 ничего не нашлось — пробую %2…")
                  .arg(current == int(YtDlp::Provider::SoundCloud) ? tr("SoundCloud")
                                                                   : tr("YouTube"),
                       other == int(YtDlp::Provider::SoundCloud) ? tr("SoundCloud")
                                                                 : tr("YouTube")));
    m_results->clear();
    m_progress->setRange(0, 0);
    m_ytdlp->search(m_searchEdit->text(), 15, static_cast<YtDlp::Provider>(other));
    return true;
}

void YouTubePanel::startJob(Job job)
{
    QString url;
    QString title;
    if (!selectedTarget(&url, &title))
        return;

    if (YtDlp::resolveBinary().isEmpty()) {
        setError(tr("yt-dlp не найден. Укажите путь в разделе «yt-dlp» слева."));
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
    const bool downloading = busy && m_ytdlp->mode() == YtDlp::Mode::Downloading;

    const QListWidgetItem *current = m_results->currentItem();
    const bool hasSelection = current != nullptr && (current->flags() & Qt::ItemIsEnabled);

    m_searchEdit->setEnabled(!busy);
    m_searchButton->setEnabled(!busy && !m_searchEdit->text().trimmed().isEmpty());
    m_results->setEnabled(!busy);
    m_playButton->setEnabled(!busy && hasSelection);
    m_saveButton->setEnabled(!busy && hasSelection);

    // Кнопка отмены нужна только во время скачивания — иначе она «висит» зря.
    m_cancelButton->setVisible(downloading);
    m_cancelButton->setEnabled(downloading);

    if (!busy && m_progress->maximum() == 0)
        m_progress->setRange(0, 100);
}

void YouTubePanel::setStatus(const QString &text)
{
    m_status->setProperty("role", "caption");
    m_status->style()->unpolish(m_status);
    m_status->style()->polish(m_status);
    m_status->setText(text);
}

void YouTubePanel::setError(const QString &text)
{
    m_status->setProperty("role", "danger");
    m_status->style()->unpolish(m_status);
    m_status->style()->polish(m_status);
    m_status->setText(text);
}

bool YouTubePanel::selectedTarget(QString *url, QString *title) const
{
    const QListWidgetItem *item = m_results->currentItem();
    if (!item || !(item->flags() & Qt::ItemIsEnabled))
        return false;

    if (url)
        *url = item->data(kUrlRole).toString();
    if (title)
        *title = item->data(kTitleRole).toString();
    return !(url && url->isEmpty());
}

void YouTubePanel::showResultsHint(const QString &text)
{
    m_results->clear();
    auto *item = new QListWidgetItem(text, m_results);
    item->setFlags(Qt::NoItemFlags);   // подсказку нельзя выбрать
    item->setForeground(QColor(0xa0, 0xa6, 0xb4));
    item->setTextAlignment(Qt::AlignCenter);
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