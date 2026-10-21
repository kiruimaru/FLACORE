#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include "../core/player/Playlist.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenFiles();
    void onPlay();
    void onPause();
    void onStop();
    void onNext();
    void onPrevious();
    void onTrackDoubleClicked(QListWidgetItem *item);
    void onRemoveTrack();
    void onClearPlaylist();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onVolumeChanged(int value);
    void onSeek(int position);
    void onPlaylistTrackAdded(int index);
    void onCurrentTrackChanged(int index);

private:
    void setupUI();
    void createMenuBar();
    void loadTrack(int index);
    void updateButtonStates();

    // Media
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    Playlist *playlist;

    // UI Components
    QWidget *centralWidget;
    QLabel *fileLabel;
    QLabel *currentTimeLabel;
    QLabel *totalTimeLabel;
    QSlider *progressSlider;
    QSlider *volumeSlider;

    // Buttons
    QPushButton *openButton;
    QPushButton *playButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QPushButton *previousButton;
    QPushButton *removeTrackButton;
    QPushButton *clearPlaylistButton;

    // Playlist UI
    QListWidget *playlistWidget;
};

#endif // MAINWINDOW_H
