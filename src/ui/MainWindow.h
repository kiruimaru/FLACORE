#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include "core/player/Playlist.h"

class YtDlp;
class YouTubePanel;
class QAction;
class QStackedWidget;

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
    void onYtDlpSettings();
    void onThemeToggle();

private:
    void setupUI();
    void createMenuBar();
    void applyTheme(bool lightTheme);
    void updateThemeButton();
    void loadTrack(int index);
    void updateButtonStates();
    void playFile(const QString &path);

    // Media
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    Playlist *playlist;
    YtDlp *ytdlp;
    YouTubePanel *youtubePanel;

    // UI Components
    QWidget *centralWidget;
    QLabel *fileLabel;
    QLabel *filePathLabel;
    QLabel *currentTimeLabel;
    QLabel *totalTimeLabel;
    QLabel *playlistCountLabel;
    QSlider *progressSlider;
    QSlider *volumeSlider;
    QStackedWidget *playlistStack;
    QStackedWidget *playPauseStack;

    // Buttons
    QPushButton *openButton;
    QPushButton *playButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QPushButton *previousButton;
    QPushButton *removeTrackButton;
    QPushButton *clearPlaylistButton;
    QPushButton *themeButton;

    // Playlist UI
    QListWidget *playlistWidget;

    QAction *darkThemeAction = nullptr;
    QAction *lightThemeAction = nullptr;
    bool m_lightTheme = false;
};

#endif // MAINWINDOW_H
