#include "PlayerBar.h"

#include "Icons.h"

#include "../theme/ThemeUtil.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QVBoxLayout>

namespace {

QString formatTime(qint64 milliseconds)
{
    if (milliseconds <= 0)
        return QStringLiteral("0:00");

    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    const auto two = [](qint64 value) {
        return QString::number(value).rightJustified(2, QLatin1Char('0'));
    };

    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours).arg(two(minutes), two(seconds));
    return QStringLiteral("%1:%2").arg(minutes).arg(two(seconds));
}

/// QLabel, который сам обрезает длинный текст многоточием.
class ElideLabel : public QLabel
{
public:
    using QLabel::QLabel;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const QFontMetrics metrics(font());
        painter.setPen(palette().color(foregroundRole()));
        painter.drawText(rect(), alignment(),
                         metrics.elidedText(text(), Qt::ElideRight, width()));
    }
};

} // namespace

PlayerBar::PlayerBar(QWidget *parent)
    : GlassPanel(parent, true)
{
    setHook(QStringLiteral("player"));
    setObjectName(QStringLiteral("playerBar"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 16, 10);
    layout->setSpacing(14);

    m_cover = new CoverArt(this);
    m_cover->setFixedSize(54, 54);
    m_cover->setRadius(ThemeUtil::number(QStringLiteral("radius-sm"), 10) + 2);
    layout->addWidget(m_cover);

    auto *meta = new QVBoxLayout();
    meta->setContentsMargins(0, 0, 0, 0);
    meta->setSpacing(2);

    m_title = new ElideLabel(tr("Ничего не играет"), this);
    m_title->setProperty("role", "section");
    m_title->setMinimumWidth(120);
    meta->addWidget(m_title);

    m_artist = new ElideLabel(tr("Откройте файлы или найдите трек на вкладке YouTube"), this);
    m_artist->setProperty("role", "caption");
    meta->addWidget(m_artist);

    layout->addLayout(meta, 3);

    auto *center = new QVBoxLayout();
    center->setContentsMargins(0, 0, 0, 0);
    center->setSpacing(4);

    m_seek = new SeekBar(this);
    m_seek->setEnabled(false);
    m_seek->setMinimumWidth(220);
    center->addWidget(m_seek);

    auto *timeRow = new QHBoxLayout();
    timeRow->setContentsMargins(0, 0, 0, 0);
    timeRow->setSpacing(8);

    m_position = new QLabel(QStringLiteral("0:00"), this);
    m_position->setProperty("role", "faint");
    m_duration = new QLabel(QStringLiteral("0:00"), this);
    m_duration->setProperty("role", "faint");

    timeRow->addWidget(m_position);
    timeRow->addStretch(1);
    timeRow->addWidget(m_duration);
    center->addLayout(timeRow);

    layout->addLayout(center, 4);

    auto *transport = new QHBoxLayout();
    transport->setContentsMargins(0, 0, 0, 0);
    transport->setSpacing(6);

    m_previous = new IconButton(Icons::Kind::Previous, QStringLiteral("transport"), 38, this);
    m_previous->setToolTip(tr("Предыдущий трек"));
    m_playPause = new IconButton(Icons::Kind::Play, QStringLiteral("transportMain"), 46, this);
    m_playPause->setToolTip(tr("Играть / пауза"));
    m_stop = new IconButton(Icons::Kind::Stop, QStringLiteral("transport"), 38, this);
    m_stop->setToolTip(tr("Стоп"));
    m_next = new IconButton(Icons::Kind::Next, QStringLiteral("transport"), 38, this);
    m_next->setToolTip(tr("Следующий трек"));

    transport->addWidget(m_previous);
    transport->addWidget(m_playPause);
    transport->addWidget(m_stop);
    transport->addWidget(m_next);
    layout->addLayout(transport);

    auto *volumeRow = new QHBoxLayout();
    volumeRow->setContentsMargins(0, 0, 0, 0);
    volumeRow->setSpacing(6);

    m_volumeButton = new IconButton(Icons::Kind::Volume, QStringLiteral("ghost"), 32, this);
    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    m_volume->setValue(70);
    m_volume->setFixedWidth(96);
    m_volume->setToolTip(tr("Громкость"));

    volumeRow->addWidget(m_volumeButton);
    volumeRow->addWidget(m_volume);
    layout->addLayout(volumeRow);

    connect(m_playPause, &QPushButton::clicked, this, &PlayerBar::playPauseRequested);
    connect(m_stop, &QPushButton::clicked, this, &PlayerBar::stopRequested);
    connect(m_next, &QPushButton::clicked, this, &PlayerBar::nextRequested);
    connect(m_previous, &QPushButton::clicked, this, &PlayerBar::previousRequested);
    connect(m_seek, &SeekBar::sliderMoved, this, &PlayerBar::seekRequested);
    connect(m_volume, &QSlider::valueChanged, this, &PlayerBar::volumeChanged);
    connect(m_volume, &QSlider::valueChanged, this, [this](int value) {
        m_volumeButton->setKind(value == 0 ? Icons::Kind::VolumeMuted : Icons::Kind::Volume);
    });

    setControlsEnabled(false);
}

void PlayerBar::setTrack(const QString &title, const QString &artist, qint64 durationMs,
                         const QImage &cover)
{
    m_title->setText(title.isEmpty() ? tr("Без названия") : title);
    m_artist->setText(artist.isEmpty() ? tr("Неизвестный исполнитель") : artist);
    m_durationMs = durationMs;
    m_positionMs = 0;

    if (cover.isNull())
        m_cover->clearImage();
    else
        m_cover->setImage(cover);

    m_seek->setRange(0, int(qMax<qint64>(0, durationMs)));
    m_seek->setValue(0);
    m_seek->setEnabled(durationMs > 0);
    updateTimeLabels();
}

void PlayerBar::clearTrack()
{
    m_title->setText(tr("Ничего не играет"));
    m_artist->setText(tr("Откройте файлы или найдите трек на вкладке YouTube"));
    m_cover->clearImage();
    m_positionMs = 0;
    m_durationMs = 0;
    m_seek->setRange(0, 0);
    m_seek->setValue(0);
    m_seek->setEnabled(false);
    updateTimeLabels();
    setPlaying(false);
}

void PlayerBar::setPosition(qint64 positionMs, qint64 durationMs)
{
    m_positionMs = positionMs;
    if (durationMs > 0 && durationMs != m_durationMs) {
        m_durationMs = durationMs;
        m_seek->setRange(0, int(durationMs));
        m_seek->setEnabled(true);
    }

    // Пока пользователь тянет ползунок, внешние обновления не мешают.
    if (!m_seek->isSliderDown())
        m_seek->setValue(int(positionMs));
    updateTimeLabels();
}

void PlayerBar::setPlaying(bool playing)
{
    m_playing = playing;
    m_playPause->setKind(playing ? Icons::Kind::Pause : Icons::Kind::Play);
}

void PlayerBar::setControlsEnabled(bool enabled)
{
    m_playPause->setEnabled(enabled);
    m_stop->setEnabled(enabled);
}

void PlayerBar::setNavigationEnabled(bool previous, bool next)
{
    m_previous->setEnabled(previous);
    m_next->setEnabled(next);
    m_previous->refreshIcon();
    m_next->refreshIcon();
}

void PlayerBar::setVolume(int volume)
{
    if (m_volume->value() == volume)
        return;
    QSignalBlocker blocker(m_volume);
    m_volume->setValue(volume);
}

int PlayerBar::volume() const
{
    return m_volume->value();
}

void PlayerBar::updateTimeLabels()
{
    m_position->setText(formatTime(m_positionMs));
    m_duration->setText(formatTime(m_durationMs));
}