#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QStyle>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);

    playlist = new Playlist(this);

    setupUI();
    createMenuBar();

    // Connections
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(playlist, &Playlist::trackAdded, this, &MainWindow::onPlaylistTrackAdded);
    connect(playlist, &Playlist::currentTrackChanged, this, &MainWindow::onCurrentTrackChanged);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    setWindowTitle("FLACORE Music Player v0.2");
    resize(900, 600);

    // Main layout - horizontal splitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(splitter);

    // Left panel - Player controls
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    fileLabel = new QLabel("No file loaded");
    fileLabel->setAlignment(Qt::AlignCenter);
    fileLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin: 10px;");
    leftLayout->addWidget(fileLabel);

    // Progress slider
    progressSlider = new QSlider(Qt::Horizontal);
    progressSlider->setRange(0, 0);
    progressSlider->setEnabled(false);
    connect(progressSlider, &QSlider::sliderMoved, this, &MainWindow::onSeek);
    leftLayout->addWidget(progressSlider);

    // Time labels
    QHBoxLayout *timeLayout = new QHBoxLayout();
    currentTimeLabel = new QLabel("00:00");
    totalTimeLabel = new QLabel("00:00");
    timeLayout->addWidget(currentTimeLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(totalTimeLabel);
    leftLayout->addLayout(timeLayout);

    // Control buttons
    QHBoxLayout *controlLayout = new QHBoxLayout();

    openButton = new QPushButton();
    openButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    openButton->setFixedSize(50, 50);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::onOpenFiles);

    previousButton = new QPushButton();
    previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    previousButton->setFixedSize(50, 50);
    previousButton->setEnabled(false);
    connect(previousButton, &QPushButton::clicked, this, &MainWindow::onPrevious);

    playButton = new QPushButton();
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playButton->setFixedSize(50, 50);
    playButton->setEnabled(false);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlay);

    pauseButton = new QPushButton();
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setFixedSize(50, 50);
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::onPause);

    stopButton = new QPushButton();
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setFixedSize(50, 50);
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStop);

    nextButton = new QPushButton();
    nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    nextButton->setFixedSize(50, 50);
    nextButton->setEnabled(false);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNext);

    controlLayout->addStretch();
    controlLayout->addWidget(openButton);
    controlLayout->addWidget(previousButton);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(pauseButton);
    controlLayout->addWidget(stopButton);
    controlLayout->addWidget(nextButton);
    controlLayout->addStretch();
    leftLayout->addLayout(controlLayout);

    // Volume control
    QHBoxLayout *volumeLayout = new QHBoxLayout();
    QLabel *volumeLabel = new QLabel("Volume:");
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    audioOutput->setVolume(0.5);
    connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(volumeSlider);
    leftLayout->addLayout(volumeLayout);

    leftLayout->addStretch();

    // Right panel - Playlist
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *playlistLabel = new QLabel("Playlist");
    playlistLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    rightLayout->addWidget(playlistLabel);

    playlistWidget = new QListWidget();
    playlistWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(playlistWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onTrackDoubleClicked);
    rightLayout->addWidget(playlistWidget);

    // Playlist buttons
    QHBoxLayout *playlistButtonLayout = new QHBoxLayout();
    removeTrackButton = new QPushButton("Remove");
    clearPlaylistButton = new QPushButton("Clear All");
    connect(removeTrackButton, &QPushButton::clicked, this, &MainWindow::onRemoveTrack);
    connect(clearPlaylistButton, &QPushButton::clicked, this, &MainWindow::onClearPlaylist);
    playlistButtonLayout->addWidget(removeTrackButton);
    playlistButtonLayout->addWidget(clearPlaylistButton);
    rightLayout->addLayout(playlistButtonLayout);

    // Add panels to splitter
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
}

void MainWindow::createMenuBar() {
    QMenu *fileMenu = menuBar()->addMenu("File");

    QAction *openAction = new QAction("Open Files", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFiles);
    fileMenu->addAction(openAction);

    QAction *quitAction = new QAction("Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(quitAction);
}

void MainWindow::onOpenFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Select Audio Files", QDir::homePath(),
        "Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a *.aac)");

    if (!files.isEmpty()) {
        playlist->addTracks(files);
        updateButtonStates();
    }
}

void MainWindow::onPlay() {
    player->play();
    playButton->setEnabled(false);
    pauseButton->setEnabled(true);
}

void MainWindow::onPause() {
    player->pause();
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
}

void MainWindow::onStop() {
    player->stop();
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    progressSlider->setValue(0);
}

void MainWindow::onNext() {
    if (playlist->hasNext()) {
        playlist->next();
        loadTrack(playlist->currentIndex());
        player->play();
    }
}

void MainWindow::onPrevious() {
    if (playlist->hasPrevious()) {
        playlist->previous();
        loadTrack(playlist->currentIndex());
        player->play();
    }
}

void MainWindow::onTrackDoubleClicked(QListWidgetItem *item) {
    int index = playlistWidget->row(item);
    playlist->setCurrentIndex(index);
    loadTrack(index);
    player->play();
}

void MainWindow::onRemoveTrack() {
    int currentRow = playlistWidget->currentRow();
    if (currentRow >= 0) {
        playlist->removeTrack(currentRow);
        delete playlistWidget->takeItem(currentRow);
        updateButtonStates();
    }
}

void MainWindow::onClearPlaylist() {
    playlist->clear();
    playlistWidget->clear();
    player->stop();
    fileLabel->setText("No file loaded");
    updateButtonStates();
}

void MainWindow::onPositionChanged(qint64 position) {
    if (!progressSlider->isSliderDown()) {
        progressSlider->setValue(position);
    }
    int seconds = (position / 1000) % 60;
    int minutes = (position / 1000) / 60;
    currentTimeLabel->setText(QString::asprintf("%02d:%02d", minutes, seconds));
}

void MainWindow::onDurationChanged(qint64 duration) {
    progressSlider->setRange(0, duration);
    progressSlider->setEnabled(duration > 0);
    int seconds = (duration / 1000) % 60;
    int minutes = (duration / 1000) / 60;
    totalTimeLabel->setText(QString::asprintf("%02d:%02d", minutes, seconds));
}

void MainWindow::onVolumeChanged(int value) {
    audioOutput->setVolume(value / 100.0);
}

void MainWindow::onSeek(int position) {
    player->setPosition(position);
}

void MainWindow::onPlaylistTrackAdded(int index) {
    Track track = playlist->getTrack(index);
    playlistWidget->addItem(track.title);
}

void MainWindow::onCurrentTrackChanged(int index) {
    playlistWidget->setCurrentRow(index);
    updateButtonStates();
}

void MainWindow::loadTrack(int index) {
    QString path = playlist->getTrackPath(index);
    if (!path.isEmpty()) {
        player->setSource(QUrl::fromLocalFile(path));
        Track track = playlist->getTrack(index);
        fileLabel->setText(track.title);
        playButton->setEnabled(true);
        stopButton->setEnabled(true);
    }
}

void MainWindow::updateButtonStates() {
    bool hasItems = playlist->count() > 0;
    playButton->setEnabled(hasItems);
    stopButton->setEnabled(hasItems);
    nextButton->setEnabled(playlist->hasNext());
    previousButton->setEnabled(playlist->hasPrevious());
}
