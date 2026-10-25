#include "CoverFetchDialog.h"

#include "core/media/MediaTags.h"
#include "core/ytdlp/YtDlp.h"
#include "theme/ThemeUtil.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

CoverFetchDialog::CoverFetchDialog(const QString &audioPath, const QString &artist,
                                   const QString &title, QWidget *parent)
    : QDialog(parent)
    , m_audioPath(audioPath)
    , m_artist(artist)
    , m_title(title)
    , m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Подтянуть фото песни из SoundCloud / YouTube"));
    resize(640, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    auto *descLabel = new QLabel(
        tr("Поиск фото для трека: <b>%1</b> — <b>%2</b><br>"
           "<small style='color: gray;'>Файл: %3</small>")
            .arg(artist.isEmpty() ? tr("Неизвестный автор") : artist,
                 title.isEmpty() ? tr("Без названия") : title,
                 audioPath),
        this);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Строка поиска
    auto *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_queryEdit = new QLineEdit(this);
    QString initialQuery;
    if (!artist.isEmpty() && !title.isEmpty())
        initialQuery = artist + QStringLiteral(" - ") + title;
    else if (!title.isEmpty())
        initialQuery = title;
    else
        initialQuery = artist;
    m_queryEdit->setText(initialQuery);
    m_queryEdit->setPlaceholderText(tr("Название трека или ссылка (SoundCloud / YouTube)"));
    searchRow->addWidget(m_queryEdit, 1);

    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItem(QStringLiteral("YouTube"), 0);
    m_providerCombo->addItem(QStringLiteral("SoundCloud"), 1);
    searchRow->addWidget(m_providerCombo);

    m_searchBtn = new QPushButton(tr("Найти фото"), this);
    m_searchBtn->setProperty("variant", "accent");
    searchRow->addWidget(m_searchBtn);

    mainLayout->addLayout(searchRow);

    // Основная область: слева список вариантов, справа превью
    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(12);

    m_resultsList = new QListWidget(this);
    m_resultsList->setUniformItemSizes(true);
    contentRow->addWidget(m_resultsList, 3);

    auto *previewCol = new QVBoxLayout();
    previewCol->setSpacing(8);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(180, 180);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(QStringLiteral("border: 1px solid rgba(255, 255, 255, 0.15); border-radius: 8px; background: rgba(0, 0, 0, 0.2);"));
    m_previewLabel->setText(tr("Нет превью"));
    previewCol->addWidget(m_previewLabel, 0, Qt::AlignCenter);

    previewCol->addStretch(1);
    contentRow->addLayout(previewCol, 2);

    mainLayout->addLayout(contentRow, 1);

    // Прогресс и статус
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(4);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(tr("Введите запрос и нажмите «Найти фото»"), this);
    m_statusLabel->setProperty("role", "caption");
    mainLayout->addWidget(m_statusLabel);

    // Кнопки действий
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);

    m_cancelBtn = new QPushButton(tr("Отмена"), this);
    btnRow->addWidget(m_cancelBtn);

    m_applyBtn = new QPushButton(tr("Установить фото в трек"), this);
    m_applyBtn->setProperty("variant", "accent");
    m_applyBtn->setEnabled(false);
    btnRow->addWidget(m_applyBtn);

    mainLayout->addLayout(btnRow);

    connect(m_searchBtn, &QPushButton::clicked, this, &CoverFetchDialog::onSearch);
    connect(m_queryEdit, &QLineEdit::returnPressed, this, &CoverFetchDialog::onSearch);
    connect(m_resultsList, &QListWidget::itemSelectionChanged, this, &CoverFetchDialog::onItemSelectionChanged);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyBtn, &QPushButton::clicked, this, &CoverFetchDialog::onApplyCover);

    // Запускаем поиск автоматически при открытии
    if (!initialQuery.isEmpty()) {
        QMetaObject::invokeMethod(this, &CoverFetchDialog::onSearch, Qt::QueuedConnection);
    }
}

CoverFetchDialog::~CoverFetchDialog()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

void CoverFetchDialog::onSearch()
{
    const QString query = m_queryEdit->text().trimmed();
    if (query.isEmpty())
        return;

    const int provider = m_providerCombo->currentData().toInt();
    startSearchProcess(query, provider);
}

void CoverFetchDialog::startSearchProcess(const QString &query, int provider)
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    const QString binary = YtDlp::resolveBinary();
    if (binary.isEmpty()) {
        m_statusLabel->setText(tr("yt-dlp не найден. Укажите путь в настройках."));
        return;
    }

    m_resultsList->clear();
    m_results.clear();
    m_previewLabel->setText(tr("Загрузка…"));
    m_previewLabel->setPixmap(QPixmap());
    m_currentImage = QImage();
    m_currentThumbnailUrl.clear();
    m_applyBtn->setEnabled(false);

    m_progressBar->setVisible(true);
    m_statusLabel->setText(tr("Поиск трека и фото на %1…")
                               .arg(provider == 1 ? QStringLiteral("SoundCloud") : QStringLiteral("YouTube")));

    QStringList args;
    if (query.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        || query.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        args << query << QStringLiteral("--no-playlist");
    } else {
        const QString prefix = (provider == 1) ? QStringLiteral("scsearch") : QStringLiteral("ytsearch");
        args << QStringLiteral("%1%2:%3").arg(prefix).arg(6).arg(query);
    }

    args << QStringLiteral("--dump-single-json")
         << QStringLiteral("--flat-playlist")
         << QStringLiteral("--skip-download")
         << QStringLiteral("--no-warnings")
         << QStringLiteral("--socket-timeout") << QStringLiteral("12");

    m_process = new QProcess(this);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
                onSearchProcessFinished(exitCode);
            });

    m_process->start(binary, args);
}

void CoverFetchDialog::onSearchProcessFinished(int exitCode)
{
    m_progressBar->setVisible(false);

    if (exitCode != 0 || !m_process) {
        m_statusLabel->setText(tr("Не удалось выполнить поиск на сервисе"));
        m_previewLabel->setText(tr("Ошибка поиска"));
        return;
    }

    const QByteArray data = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_statusLabel->setText(tr("Ничего не найдено"));
        m_previewLabel->setText(tr("Ничего не найдено"));
        return;
    }

    const QJsonObject root = doc.object();
    QList<YtDlpResult> results;

    const auto parseEntry = [&results](const QJsonObject &entry) {
        if (entry.isEmpty())
            return;
        YtDlpResult res;
        res.id = entry.value(QStringLiteral("id")).toString();
        res.title = entry.value(QStringLiteral("title")).toString();
        res.uploader = entry.value(QStringLiteral("uploader")).toString();
        if (res.uploader.isEmpty())
            res.uploader = entry.value(QStringLiteral("channel")).toString();

        const QJsonArray thumbnails = entry.value(QStringLiteral("thumbnails")).toArray();
        if (!thumbnails.isEmpty()) {
            res.thumbnailUrl = thumbnails.last().toObject().value(QStringLiteral("url")).toString();
        }
        if (res.thumbnailUrl.isEmpty()) {
            res.thumbnailUrl = entry.value(QStringLiteral("thumbnail")).toString();
        }
        if (!res.title.isEmpty() && !res.thumbnailUrl.isEmpty())
            results.append(res);
    };

    if (root.contains(QStringLiteral("entries")) && root.value(QStringLiteral("entries")).isArray()) {
        const QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
        for (const QJsonValue &val : entries)
            parseEntry(val.toObject());
    } else {
        parseEntry(root);
    }

    m_results = results;

    if (m_results.isEmpty()) {
        m_statusLabel->setText(tr("Ничего не найдено или у треков нет фото"));
        m_previewLabel->setText(tr("Нет фото"));
        return;
    }

    m_statusLabel->setText(tr("Найдено вариантов: %1. Выберите подходящий.").arg(m_results.size()));

    for (const YtDlpResult &res : m_results) {
        const QString itemText = QStringLiteral("%1 — %2").arg(res.uploader, res.title);
        m_resultsList->addItem(itemText);
    }

    if (m_resultsList->count() > 0)
        m_resultsList->setCurrentRow(0);
}

void CoverFetchDialog::onItemSelectionChanged()
{
    const int row = m_resultsList->currentRow();
    if (row < 0 || row >= m_results.size()) {
        m_applyBtn->setEnabled(false);
        return;
    }

    const YtDlpResult &selected = m_results.at(row);
    if (selected.thumbnailUrl.isEmpty()) {
        m_previewLabel->setText(tr("Нет URL фото"));
        m_applyBtn->setEnabled(false);
        return;
    }

    downloadPreview(selected.thumbnailUrl);
}

void CoverFetchDialog::downloadPreview(const QString &url)
{
    m_currentThumbnailUrl = url;
    m_previewLabel->setText(tr("Загрузка фото…"));
    m_applyBtn->setEnabled(false);

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_previewLabel->setText(tr("Ошибка загрузки фото"));
            return;
        }

        // Если пока качалось, пользователь уже переключился на другой трек — игнорируем
        if (m_currentThumbnailUrl != url)
            return;

        const QByteArray data = reply->readAll();
        QImage image;
        if (!image.loadFromData(data) || image.isNull()) {
            m_previewLabel->setText(tr("Не удалось декодировать фото"));
            return;
        }

        m_currentImage = image;
        const QPixmap pixmap = QPixmap::fromImage(
            image.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_previewLabel->setPixmap(pixmap);
        m_applyBtn->setEnabled(true);
        m_statusLabel->setText(tr("Фото готово: %1x%2. Нажмите «Установить фото в трек».").arg(image.width()).arg(image.height()));
    });
}

void CoverFetchDialog::onApplyCover()
{
    if (m_currentImage.isNull() || m_audioPath.isEmpty())
        return;

    m_applyBtn->setEnabled(false);
    m_statusLabel->setText(tr("Установка фото в аудиофайл…"));

    const QString ffmpegDir = YtDlp::resolveFfmpegDir();
    const bool ok = MediaTags::saveCoverToFile(m_audioPath, m_currentImage, ffmpegDir);

    if (ok) {
        emit coverApplied(m_currentImage);
        accept();
    } else {
        m_statusLabel->setText(tr("Ошибка сохранения фото в файл"));
        m_applyBtn->setEnabled(true);
    }
}
