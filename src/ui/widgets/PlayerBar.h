#ifndef PLAYERBAR_H
#define PLAYERBAR_H

#include "GlassWidgets.h"

#include <QImage>

class QLabel;
class QSlider;

class IconButton;
class SeekBar;

/**
 * Нижняя панель плеера: обложка, трек, транспорт, перемотка, громкость.
 * Собственного состояния не хранит — только показывает то, что дал MainWindow,
 * и сообщает наружу о нажатиях.
 */
class PlayerBar : public GlassPanel
{
    Q_OBJECT

public:
    explicit PlayerBar(QWidget *parent = nullptr);

    void setTrack(const QString &title, const QString &artist, qint64 durationMs,
                  const QImage &cover);
    void clearTrack();
    void setPosition(qint64 positionMs, qint64 durationMs);
    void setPlaying(bool playing);
    void setControlsEnabled(bool enabled);
    void setNavigationEnabled(bool previous, bool next);
    void setRepeat(bool repeat);
    bool isRepeat() const { return m_repeatActive; }
    void setVolume(int volume);
    int volume() const;

    SeekBar *seekBar() const { return m_seek; }

signals:
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void repeatToggled(bool active);
    void seekRequested(int positionMs);
    void volumeChanged(int value);

private:
    void updateTimeLabels();

    CoverArt *m_cover = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_artist = nullptr;
    QLabel *m_position = nullptr;
    QLabel *m_duration = nullptr;
    SeekBar *m_seek = nullptr;
    QSlider *m_volume = nullptr;
    IconButton *m_volumeButton = nullptr;
    IconButton *m_previous = nullptr;
    IconButton *m_playPause = nullptr;
    IconButton *m_stop = nullptr;
    IconButton *m_next = nullptr;
    IconButton *m_repeat = nullptr;

    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_playing = false;
    bool m_repeatActive = false;
};

#endif // PLAYERBAR_H