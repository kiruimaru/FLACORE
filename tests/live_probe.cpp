// Живой прогон «Обзора»: настоящая сеть и настоящий yt-dlp.
//
// Это НЕ часть ctest — тесты по умолчанию офлайновые. Здесь проверяется то,
// что нельзя проверить на фикстурах: реально ли приходят чарты Deezer и Apple,
// играет ли превью-поток, доезжает ли выбранная строка чарта до плеера
// (поиск → скачивание в кэш → сигнал playRequested → плейлист) и читается ли
// чарт SoundCloud (это зависит от сети: из РФ SoundCloud отдаёт 404).
//
// Запуск (нужен интернет и yt-dlp):
//   ./live_probe.exe
// Переменные:
//   FLACORE_LIVE_YTDLP — путь к yt-dlp, если он не в PATH.
#include "core/online/Charts.h"
#include "core/player/Playlist.h"
#include "ui/MainWindow.h"
#include "ui/pages/DiscoverPage.h"
#include "ui/widgets/ShelfDelegate.h"

#include <QApplication>
#include <QAudioOutput>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMediaPlayer>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <cstdio>

namespace {

int g_failures = 0;

void check(bool condition, const QString &what)
{
    std::printf("  %s %s\n", condition ? "ok  " : "FAIL", what.toUtf8().constData());
    if (!condition)
        ++g_failures;
}

void note(const QString &what)
{
    std::printf("  --   %s\n", what.toUtf8().constData());
}

void pump(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(5);
    }
}

template <typename Predicate>
bool waitFor(Predicate predicate, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs)
        pump(50);
    return predicate();
}

QString rowText(const QListWidget *list, int row)
{
    if (!list || list->count() <= row)
        return QString();
    const QListWidgetItem *item = list->item(row);
    return QStringLiteral("%1 — %2  [%3]")
        .arg(item->data(ShelfDelegate::ArtistRole).toString(), item->text(),
             item->data(ShelfDelegate::DurationRole).toString());
}

/// Открыть строку так же, как пользователь: двойной клик.
void activateRow(QListWidget *list, int row)
{
    list->setCurrentRow(row);
    QMetaObject::invokeMethod(list, "itemDoubleClicked", Qt::DirectConnection,
                              Q_ARG(QListWidgetItem *, list->item(row)));
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("live-probe"));
    QStandardPaths::setTestModeEnabled(true);

    const QByteArray binary = qgetenv("FLACORE_LIVE_YTDLP");
    if (!binary.isEmpty()) {
        QSettings settings;
        settings.setValue(QStringLiteral("ytdlp/binary"), QString::fromLocal8Bit(binary));
        settings.sync();
    }

    std::printf("Живой прогон «Обзора» (сеть + yt-dlp)\n\n");

    MainWindow window;
    window.resize(1180, 730);
    window.show();          // открывается «Обзор» — он же первая страница
    pump(1500);

    auto *discover = window.findChild<DiscoverPage *>();
    check(discover != nullptr, QStringLiteral("страница «Обзор» в окне"));
    if (!discover) {
        std::printf("\nПРОВАЛ: страницы нет\n");
        return 1;
    }

    auto *status = window.findChild<QLabel *>(QStringLiteral("ytStatus"));
    auto *playlist = window.findChild<Playlist *>();

    // --- 1. Deezer: живой топ ----------------------------------------------
    std::printf("Deezer (живой запрос):\n");
    QListWidget *deezer = discover->shelfList(DiscoverPage::deezerShelfId());
    const bool deezerLoaded = waitFor(
        [discover]() { return discover->shelfTrackCount(DiscoverPage::deezerShelfId()) > 0; }, 25000);
    check(deezerLoaded, QStringLiteral("полка Deezer заполнилась из сети"));
    if (deezerLoaded) {
        std::printf("       первая строка: %s\n", rowText(deezer, 0).toUtf8().constData());
        check(discover->shelfTrackCount(DiscoverPage::deezerShelfId()) >= 10,
              QStringLiteral("в топе Deezer не меньше 10 треков (пришло %1)")
                  .arg(discover->shelfTrackCount(DiscoverPage::deezerShelfId())));
    }

    // --- 2. Apple Music: живой топ -----------------------------------------
    std::printf("\nApple Music (живой запрос):\n");
    QListWidget *apple = discover->shelfList(DiscoverPage::appleShelfId());
    const bool appleLoaded = waitFor(
        [discover]() { return discover->shelfTrackCount(DiscoverPage::appleShelfId()) > 0; }, 25000);
    check(appleLoaded, QStringLiteral("полка Apple Music заполнилась из сети"));
    if (appleLoaded) {
        std::printf("       первая строка: %s\n", rowText(apple, 0).toUtf8().constData());
        check(discover->shelfTrackCount(DiscoverPage::appleShelfId()) >= 10,
              QStringLiteral("в топе Apple не меньше 10 треков (пришло %1)")
                  .arg(discover->shelfTrackCount(DiscoverPage::appleShelfId())));
    }

    // --- 3. Превью: играет ли реальный поток --------------------------------
    std::printf("\nПревью (поток из сети):\n");
    {
        QString preview;
        for (int row = 0; row < (deezer ? deezer->count() : 0) && preview.isEmpty(); ++row)
            preview = deezer->item(row)->data(ShelfDelegate::PreviewUrlRole).toString();

        if (preview.isEmpty()) {
            note(QStringLiteral("превью в ответе нет — пропускаем"));
        } else {
            QMediaPlayer player;
            QAudioOutput output;
            player.setAudioOutput(&output);
            player.setSource(QUrl(preview));
            player.play();

            const bool playing = waitFor([&player]() { return player.position() > 1500; }, 20000);
            check(playing, QStringLiteral("превью реально играет (позиция %1 мс)")
                               .arg(player.position()));
            player.stop();
        }
    }

    // --- 4. «Слушать» из чарта: поиск → скачивание → плеер ------------------
    std::printf("\nСлушать из чарта Deezer (поиск → скачивание → плеер):\n");
    if (deezerLoaded) {
        const int before = playlist ? playlist->count() : -1;
        const QString clicked = rowText(deezer, 0);
        std::printf("       строка: %s\n", clicked.toUtf8().constData());
        activateRow(deezer, 0);

        // Печатаем каждый новый статус панели: видно порядок попыток и источников.
        QString lastStatus;
        const bool arrived = waitFor(
            [&]() {
                if (status && status->text() != lastStatus) {
                    lastStatus = status->text();
                    std::printf("       статус: %s\n", lastStatus.toUtf8().constData());
                }
                return playlist && playlist->count() > before;
            },
            240000);

        check(arrived, QStringLiteral("строка чарта доехала до плейлиста"));

        if (playlist && playlist->count() > before) {
            const Track track = playlist->getTrack(playlist->count() - 1);
            const QFileInfo info(track.filePath);
            std::printf("       файл: %s\n", track.filePath.toUtf8().constData());
            check(info.exists() && info.size() > 0,
                  QStringLiteral("файл скачан (%1 КБ)").arg(info.size() / 1024));

            auto *media = window.findChild<QMediaPlayer *>();
            // QMediaPlayer отдаёт путь с прямыми слэшами, плейлист — как в системе.
            const QString source = media
                ? QDir::fromNativeSeparators(media->source().toLocalFile())
                : QString();
            const QString expected = QDir::fromNativeSeparators(track.filePath);
            std::printf("       источник плеера: %s\n", source.toUtf8().constData());
            check(media && source == expected,
                  QStringLiteral("плеер поставлен на этот файл"));
            if (media && source != expected)
                std::printf("       ждали: %s\n", expected.toUtf8().constData());
        }
    }

    // --- 5. Чарт SoundCloud: только если сеть его отдаёт --------------------
    std::printf("\nЧарт SoundCloud (через yt-dlp):\n");
    {
        QListWidget *soundCloud = discover->shelfList(DiscoverPage::soundCloudShelfId());
        discover->refreshAll();       // заодно повторно тянет Deezer и Apple

        const bool chartLoaded = waitFor(
            [discover]() { return discover->shelfTrackCount(DiscoverPage::soundCloudShelfId()) > 0; },
            90000);

        if (chartLoaded) {
            check(true, QStringLiteral("чарт SoundCloud пришёл (%1 записей)")
                            .arg(discover->shelfTrackCount(DiscoverPage::soundCloudShelfId())));
            std::printf("       первая строка: %s\n", rowText(soundCloud, 0).toUtf8().constData());
        } else {
            // Из РФ soundcloud.com/charts отдаёт 404: это не ошибка приложения.
            note(QStringLiteral("SoundCloud недоступен из этой сети (нужен VPN/прокси) — "
                                "полка показывает понятную ошибку"));
        }
    }

    std::printf("\n%s (провалов: %d)\n",
                g_failures == 0 ? "ЖИВОЙ ПРОГОН: ОК" : "ЖИВОЙ ПРОГОН: ЕСТЬ ОШИБКИ", g_failures);
    return g_failures == 0 ? 0 : 1;
}
