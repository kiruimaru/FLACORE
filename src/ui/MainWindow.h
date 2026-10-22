#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>

#include "core/online/Charts.h"
#include "core/ytdlp/YtDlpTypes.h"

class QLabel;
class QListWidget;
class QPoint;
class QResizeEvent;
class QStackedWidget;
class QTimer;

class AuroraBackdrop;
class AppearancePage;
class DiscoverPage;
class LibraryPage;
class PlayerBar;
class Playlist;
class TitleBar;
class YouTubePanel;
class YtDlp;
class YtDlpPage;

/**
 * Каркас приложения: шапка, сайдбар с разделами, страницы по центру и плавающая
 * панель плеера снизу. Владеет состоянием (Playlist, QMediaPlayer, YtDlp) и
 * связывает его с представлением; страницы ничего не знают друг о друге.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onOpenFiles();
    void onPlayPause();
    void onStop();
    void onNext();
    void onPrevious();
    void onSeek(int positionMs);
    void onVolumeChanged(int value);
    void onTrackActivated(int row);
    void onPlaylistTrackAdded(int index);
    void onPlaylistTrackRemoved(int index);
    void onCurrentTrackChanged(int index);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onMediaError(QMediaPlayer::Error error, const QString &errorString);
    void onShowMenu(const QPoint &globalPos);
    void onYtDlpSettings();
    void onNavChanged(int row);
    void setFrameless(bool frameless);
    void toggleMaximize();

private:
    void setupUi();
    void setupActions();
    void wireCore();
    /// Строка чарта: искать в yt-dlp и включить/сохранить первый результат.
    void routeChartTrack(const ChartTrack &track, bool saveToLibrary);
    /// Прослушать 30-секундное превью прямо из сети (Deezer/Apple).
    void playChartPreview(const ChartTrack &track);
    /// Прочитать чарт SoundCloud через yt-dlp.
    void requestSoundCloudChart(const QString &url);
    void addSoundCloudChart(const QList<YtDlpResult> &results);
    void applyFrameless(bool frameless);
    void loadTrack(int index, bool autoplay);
    void addPaths(const QStringList &paths, bool fromDrop);
    void notify(const QString &text, bool error = false);
    void updateNavIcons();
    void refreshTransport();
    void positionToast();

    // каркас
    AuroraBackdrop *m_backdrop = nullptr;
    TitleBar *m_titleBar = nullptr;
    QListWidget *m_nav = nullptr;
    QStackedWidget *m_pages = nullptr;
    PlayerBar *m_playerBar = nullptr;
    QLabel *m_toast = nullptr;
    QTimer *m_toastTimer = nullptr;

    // страницы
    DiscoverPage *m_discover = nullptr;
    LibraryPage *m_library = nullptr;
    YouTubePanel *m_youtube = nullptr;
    AppearancePage *m_appearance = nullptr;
    YtDlpPage *m_ytdlpPage = nullptr;

    // ядро
    QMediaPlayer *m_media = nullptr;
    QAudioOutput *m_audio = nullptr;
    Playlist *m_playlist = nullptr;
    YtDlp *m_ytdlp = nullptr;

    int m_playingIndex = -1;
    bool m_chartSearchPending = false;   // чарт SoundCloud читает yt-dlp
    bool m_previewPlaying = false;       // играет превью, а не трек плейлиста
    int m_inset = 0;
    bool m_frameless = false;
    bool m_userTransport = false;
};

#endif // MAINWINDOW_H