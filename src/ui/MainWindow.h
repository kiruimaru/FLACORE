#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPixmap>
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
class QResizeEvent;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

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
    void applyTheme(const QString &theme);
    void updateThemeButton();
    void loadTrack(int index);
    void updateButtonStates();
    void playFile(const QString &path);
    void applyBackground();
    void editThemeHardcore();
    void editThemeInteractive();
    void exportThemeConfig();
    void importThemeConfig();
    void chooseBackground();

    // Media
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    Playlist *playlist;
    YtDlp *ytdlp;
    YouTubePanel *youtubePanel;

    // UI Components
    QWidget *centralWidget = nullptr;
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
    QList<QAction *> m_themeActions;
    QString m_theme = QStringLiteral("dark");
    QPixmap m_backgroundImage;
};

#endif // MAINWINDOW_H
