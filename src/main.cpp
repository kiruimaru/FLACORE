#include <QApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Простое окно
    QWidget window;
    window.setWindowTitle("FLACORE Music Player v0.1");
    window.resize(400, 200);

    QVBoxLayout *layout = new QVBoxLayout(&window);
    QLabel *label = new QLabel("Welcome to FLACORE!", &window);
    QPushButton *button = new QPushButton("Open Audio File", &window);

    layout->addWidget(label);
    layout->addWidget(button);

    // Qt6 медиаплеер
    QMediaPlayer *player = new QMediaPlayer(&window);
    QAudioOutput *audioOutput = new QAudioOutput(&window);
    player->setAudioOutput(audioOutput);

    // Открыть файл и воспроизвести
    QObject::connect(button, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&window,
            "Select Audio File", "", "Audio Files (*.mp3 *.wav *.flac *.ogg)");
        if (!fileName.isEmpty()) {
            player->setSource(QUrl::fromLocalFile(fileName));
            player->play();
            label->setText("Playing: " + fileName);
            std::cout << "Playing: " << fileName.toStdString() << std::endl;
        }
    });

    window.show();
    return app.exec();
}