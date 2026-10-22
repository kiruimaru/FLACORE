#include "YtDlpPage.h"

#include "../YtDlpSettingsForm.h"
#include "../widgets/GlassWidgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QVBoxLayout>

YtDlpPage::YtDlpPage(QWidget *parent)
    : QWidget(parent)
{
    setProperty("dataUi", "main");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Загрузка через yt-dlp"), this);
    title->setProperty("role", "title");
    layout->addWidget(title);

    auto *subtitle = new QLabel(tr("Пути к бинарникам и формат сохранения. "
                                   "Скачивание и прослушивание работают без ffmpeg, "
                                   "конвертация в FLAC/MP3 — нет."),
                                this);
    subtitle->setProperty("role", "caption");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto *card = new GlassPanel(this);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);

    m_form = new YtDlpSettingsForm(card);
    cardLayout->addWidget(m_form);

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(8);

    auto *saveButton = new QPushButton(tr("Сохранить"), card);
    saveButton->setProperty("variant", "accent");
    auto *checkButton = new QPushButton(tr("Проверить yt-dlp"), card);
    buttons->addWidget(saveButton);
    buttons->addWidget(checkButton);
    buttons->addStretch(1);
    cardLayout->addLayout(buttons);

    m_status = new QLabel(card);
    m_status->setProperty("role", "caption");
    m_status->setWordWrap(true);
    cardLayout->addWidget(m_status);

    layout->addWidget(card);
    layout->addStretch(1);

    connect(saveButton, &QPushButton::clicked, this, &YtDlpPage::save);
    connect(checkButton, &QPushButton::clicked, this, &YtDlpPage::checkRequested);
}

void YtDlpPage::showStatus(const QString &text, bool error)
{
    m_status->setProperty("role", error ? "danger" : "caption");
    m_status->style()->unpolish(m_status);
    m_status->style()->polish(m_status);
    m_status->setText(text);
}

void YtDlpPage::save()
{
    QSettings settings;
    settings.setValue(QStringLiteral("ytdlp/binary"), m_form->binaryPath());
    settings.setValue(QStringLiteral("ytdlp/ffmpegDir"), m_form->ffmpegDir());
    settings.setValue(QStringLiteral("ytdlp/libraryDir"), m_form->libraryDir());
    settings.setValue(QStringLiteral("ytdlp/audioFormat"), m_form->audioFormat());
    settings.sync();

    showStatus(tr("Настройки сохранены"));
    emit applied(m_form->binaryPath());
}