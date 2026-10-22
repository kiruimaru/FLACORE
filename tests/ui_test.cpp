// Интерфейсный тест (offscreen): каркас окна, связь плейлиста с представлением,
// переключение всех встроенных тем, свой CSS и аварийный выход.
// Плюс сохраняет скриншоты, чтобы вёрстку можно было посмотреть глазами.
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>

#include "core/online/Charts.h"
#include "core/player/Playlist.h"
#include "ui/MainWindow.h"
#include "ui/pages/AppearancePage.h"
#include "ui/pages/DiscoverPage.h"
#include "ui/pages/LibraryPage.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeUtil.h"
#include "ui/widgets/AuroraBackdrop.h"
#include "ui/widgets/PlayerBar.h"
#include "ui/widgets/ShelfDelegate.h"
#include "ui/widgets/TrackDelegate.h"

namespace {

int g_failures = 0;

void check(bool condition, const QString &what)
{
    std::printf("  %s %s\n", condition ? "ok  " : "FAIL", what.toUtf8().constData());
    if (!condition)
        ++g_failures;
}

void info(const QString &what)
{
    std::printf("       %s\n", what.toUtf8().constData());
}

void putLE32(QByteArray &out, quint32 value)
{
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 24) & 0xFF));
}

/// Настоящий WAV (1 секунда) — чтобы длительность читалась из файла.
QByteArray wav()
{
    QByteArray out;
    const quint32 dataSize = 8000;
    out.append("RIFF");
    putLE32(out, 36 + dataSize);
    out.append("WAVE");
    out.append("fmt ");
    putLE32(out, 16);
    out.append(char(1)).append(char(0));
    out.append(char(1)).append(char(0));
    putLE32(out, 8000);
    putLE32(out, 8000);
    out.append(char(1)).append(char(0));
    out.append(char(8)).append(char(0));
    out.append("data");
    putLE32(out, dataSize);
    out.append(QByteArray(int(dataSize), char(0x80)));
    return out;
}

QString makeWav(const QString &dir, const QString &name)
{
    const QString path = QDir(dir).filePath(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
        file.write(wav());
    return path;
}

void pump(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(5);
    }
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("ui-test"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.1"));
    QStandardPaths::setTestModeEnabled(true);

    // Тест должен быть герметичным: чистим настройки от прошлых прогонов,
    // иначе сохранённый «свой CSS» приедет в новую сессию.
    QSettings().clear();
    QSettings().sync();

    // Ресурсы лежат в статической библиотеке.
    Q_INIT_RESOURCE(resources);

    QTemporaryDir dir;
    if (!dir.isValid())
        return 2;

    // Тесты не ходят в сеть: «Обзор» не запрашивает чарты Deezer и Apple.
    qputenv("FLACORE_OFFLINE", "1");

    std::printf("Интерфейс: каркас, плейлист, темы\n");

    ThemeManager themes;
    themes.apply();

    // --- каталог тем --------------------------------------------------------
    const QList<Theme> builtin = themes.themes();
    check(builtin.size() >= 5, QStringLiteral("встроенных тем найдено: %1").arg(builtin.size()));
    for (const Theme &theme : builtin)
        info(QStringLiteral("тема: %1 (%2), акцент %3%4")
                 .arg(theme.name, theme.id, theme.accent,
                      theme.builtin ? QString() : QStringLiteral(", пользовательская")));

    // --- безрамочный режим читается из настроек -----------------------------
    AppearancePage::setFramelessEnabled(true);

    MainWindow window;
    window.resize(1180, 730);
    window.show();
    pump(200);

    check(window.windowFlags().testFlag(Qt::FramelessWindowHint),
          QStringLiteral("безрамочный режим включён"));
    check(window.testAttribute(Qt::WA_TranslucentBackground),
          QStringLiteral("фон окна прозрачный (для скругления и тени)"));

    auto *nav = window.findChild<QListWidget *>(QStringLiteral("sidebarNav"));
    auto *pages = window.findChild<QStackedWidget *>();
    auto *library = window.findChild<LibraryPage *>();
    auto *playerBar = window.findChild<PlayerBar *>(QStringLiteral("playerBar"));
    auto *backdrop = window.findChild<AuroraBackdrop *>();
    auto *playlist = window.findChild<Playlist *>();

    check(nav && nav->count() == 5, QStringLiteral("в сайдбаре 5 разделов"));
    check(pages && pages->count() == 5, QStringLiteral("в стеке 5 страниц"));
    check(library && playerBar && backdrop, QStringLiteral("библиотека, панель плеера и подложка на месте"));
    check(playlist != nullptr, QStringLiteral("плейлист найден как владелец состояния"));
    if (!nav || !pages || !library || !playerBar || !backdrop || !playlist)
        return 2;

    check(nav->currentRow() == 0 && pages->currentIndex() == 0,
          QStringLiteral("на старте открыт раздел «Обзор»"));
    check(nav->item(1)->text() == QStringLiteral("Библиотека")
              && nav->item(2)->text() == QLatin1String("YouTube"),
          QStringLiteral("порядок разделов: Обзор, Библиотека, YouTube, …"));

    // --- иконки транспорта: на выключенной кнопке должны быть различимы ------
    std::printf("\nТранспорт (иконки):\n");
    {
        QPushButton *mainButton = nullptr;
        for (QPushButton *button : window.findChildren<QPushButton *>()) {
            if (button->property("dataUi").toString() == QLatin1String("transportMain")) {
                mainButton = button;
                break;
            }
        }
        check(mainButton != nullptr, QStringLiteral("главная кнопка транспорта найдена"));

        if (mainButton) {
            // Средний цвет непрозрачных пикселей иконки: показывает, каким токеном
            // она нарисована, без оценки «на глаз».
            const auto iconColor = [](const QIcon &icon) {
                const QImage image = icon.pixmap(18, 18).toImage();
                qint64 r = 0;
                qint64 g = 0;
                qint64 b = 0;
                qint64 n = 0;
                for (int y = 0; y < image.height(); ++y) {
                    for (int x = 0; x < image.width(); ++x) {
                        const QColor pixel = image.pixelColor(x, y);
                        if (pixel.alpha() < 40)
                            continue;
                        r += pixel.red();
                        g += pixel.green();
                        b += pixel.blue();
                        ++n;
                    }
                }
                return n == 0 ? QColor() : QColor(int(r / n), int(g / n), int(b / n));
            };
            const auto distance = [](const QColor &a, const QColor &b) {
                return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
                       + qAbs(a.blue() - b.blue());
            };

            mainButton->setEnabled(false);
            pump(30);
            const QColor disabledColor = iconColor(mainButton->icon());

            mainButton->setEnabled(true);
            pump(30);
            const QColor enabledColor = iconColor(mainButton->icon());

            check(disabledColor.isValid() && enabledColor.isValid(),
                  QStringLiteral("иконка главной кнопки нарисована"));
            check(disabledColor != enabledColor,
                  QStringLiteral("цвет иконки меняется вместе с состоянием кнопки"));

            // Раньше выключенная кнопка оставалась с белой иконкой на светлом фоне.
            const QColor contrast
                = ThemeUtil::color(QStringLiteral("accent-contrast"), Qt::white);
            const QColor muted = ThemeUtil::color(QStringLiteral("text-muted"), Qt::gray);
            check(distance(disabledColor, muted) < distance(disabledColor, contrast),
                  QStringLiteral("на выключенной кнопке иконка ближе к text-muted, "
                                 "чем к accent-contrast"));

            mainButton->setEnabled(false);
            pump(20);
        }
    }

    // --- добавление треков через перетаскивание ----------------------------
    const QStringList dropped = {makeWav(dir.path(), QStringLiteral("one.wav")),
                                 makeWav(dir.path(), QStringLiteral("two.wav")),
                                 makeWav(dir.path(), QStringLiteral("three.wav"))};

    QMetaObject::invokeMethod(library, "filesDropped", Q_ARG(QStringList, dropped));
    pump(100);

    check(playlist->count() == 3, QStringLiteral("модель: 3 трека после перетаскивания"));
    check(library->list()->count() == 3, QStringLiteral("список: 3 строки после перетаскивания"));
    check(library->list()->item(0)->text() == QLatin1String("one.wav"),
          QStringLiteral("название строки = имени файла"));
    check(library->list()->item(0)->data(TrackDelegate::DurationRole).toString()
              == QLatin1String("0:01"),
          QStringLiteral("длительность прочитана из файла (0:01)"));

    // Повторное перетаскивание тех же файлов не должно добавлять дубли.
    QMetaObject::invokeMethod(library, "filesDropped", Q_ARG(QStringList, dropped));
    pump(100);
    check(playlist->count() == 3, QStringLiteral("дубликаты не добавлены (3 трека)"));

    // --- переключение трека -------------------------------------------------
    int currentChanges = 0;
    int removals = 0;
    int cleared = 0;
    QObject::connect(playlist, &Playlist::currentTrackChanged, [&currentChanges](int) {
        ++currentChanges;
    });
    QObject::connect(playlist, &Playlist::trackRemoved, [&removals](int) { ++removals; });
    QObject::connect(playlist, &Playlist::playlistCleared, [&cleared]() { ++cleared; });

    playlist->setCurrentIndex(2);
    pump(50);
    check(playlist->currentIndex() == 2, QStringLiteral("выбран третий трек"));
    check(library->delegate()->playingRow() == 2,
          QStringLiteral("строка «играет сейчас» совпадает с моделью"));
    check(playerBar->isVisible(), QStringLiteral("панель плеера видима"));
    check(currentChanges > 0, QStringLiteral("сигнал смены трека дошёл до представления"));

    // --- удаление: сигналы и сдвиг индексов ---------------------------------
    check(removals == 0, QStringLiteral("до удаления сигналов trackRemoved не было"));
    playlist->removeTrack(1);
    pump(50);
    check(removals == 1, QStringLiteral("trackRemoved пришёл (раньше его никто не слушал)"));
    check(playlist->count() == 2 && library->list()->count() == 2,
          QStringLiteral("модель и список сократились вместе"));
    check(playlist->currentIndex() == 1, QStringLiteral("индекс играющего трека сдвинулся на 1"));
    check(library->delegate()->playingRow() == 1,
          QStringLiteral("подсветка «играет сейчас» переехала вместе с индексом"));

    // Удаление самого играющего трека: плеер обязан перестать его показывать.
    playlist->setCurrentIndex(1);
    pump(30);
    const int playingBefore = playlist->currentIndex();
    playlist->removeTrack(playingBefore);
    pump(80);
    check(playlist->currentIndex() == -1, QStringLiteral("после удаления играющего трека индекс = -1"));
    check(library->delegate()->playingRow() == -1,
          QStringLiteral("подсветка снята (плеер не показывает удалённый трек)"));

    playlist->clear();
    pump(50);
    check(cleared == 1 && playlist->count() == 0 && library->list()->count() == 0,
          QStringLiteral("очистка: сигнал плюс пустые модель и список"));

    // --- все встроенные темы на живом окне ---------------------------------
    std::printf("\nПереключение тем:\n");
    const QString shotsDir = QDir(QDir::currentPath()).filePath(QStringLiteral("screenshots"));
    QDir().mkpath(shotsDir);

    for (const Theme &theme : themes.themes()) {
        themes.setCurrentTheme(theme.id);
        pump(120);

        const QString accent = themes.tokens().accent.name(QColor::HexRgb);
        const bool inQss = app.styleSheet().contains(accent, Qt::CaseInsensitive);
        const bool inPalette = app.palette().color(QPalette::Highlight).name(QColor::HexRgb)
                               == accent;
        check(inQss && inPalette,
              QStringLiteral("тема «%1»: акцент %2 в QSS и в палитре").arg(theme.name, accent));
        check(themes.lastProblems().isEmpty(),
              QStringLiteral("тема «%1» применилась без замечаний").arg(theme.name));
        check(window.findChild<PlayerBar *>(QStringLiteral("playerBar")) != nullptr,
              QStringLiteral("тема «%1»: каркас цел").arg(theme.name));

        // Скриншот для глаз: пустой экран малоинтересен, поэтому с треками.
        if (theme.id == QLatin1String("liquid-glass") || theme.id == QLatin1String("sugar-cloud")
            || theme.id == QLatin1String("neon")) {
            const QStringList shots = {makeWav(dir.path(), QStringLiteral("Arrival.wav")),
                                       makeWav(dir.path(), QStringLiteral("Late Night Mix.wav")),
                                       makeWav(dir.path(), QStringLiteral("Rain on Glass.wav")),
                                       makeWav(dir.path(), QStringLiteral("Finale.wav"))};
            QMetaObject::invokeMethod(library, "filesDropped", Q_ARG(QStringList, shots));
            playlist->setCurrentIndex(1);
            pump(150);

            const QString file = QDir(shotsDir).filePath(theme.id + QStringLiteral(".png"));
            const bool saved = window.grab().save(file);
            check(saved, QStringLiteral("скриншот темы «%1» сохранён").arg(theme.name));
            if (saved)
                info(file);

            playlist->clear();
            pump(60);
        }
    }

    // --- «Обзор»: живая подборка -------------------------------------------
    std::printf("\nОбзор (живая подборка):\n");
    auto *discover = window.findChild<DiscoverPage *>();
    check(discover != nullptr, QStringLiteral("страница «Обзор» найдена"));
    if (discover) {
        check(discover->shelfCount() == 3,
              QStringLiteral("в «Обзоре» три полки: Deezer, Apple Music и SoundCloud"));

        // Данные подставляем сами: сеть в тестах не участвует.
        DiscoverPage::Shelf shelf;
        shelf.id = DiscoverPage::deezerShelfId();
        shelf.title = QStringLiteral("Топ Deezer");
        shelf.subtitle = QStringLiteral("тестовые данные, 5 треков");
        for (int i = 0; i < 5; ++i) {
            ChartTrack track;
            track.title = QStringLiteral("Тестовый трек %1").arg(i + 1);
            track.artist = QStringLiteral("Исполнитель %1").arg(i + 1);
            track.source = QStringLiteral("deezer");
            track.url = QStringLiteral("https://example.com/track/%1").arg(i + 1);
            track.coverUrl = QStringLiteral("https://example.com/cover%1.jpg").arg(i + 1);
            track.previewUrl = QStringLiteral("https://example.com/preview%1.mp3").arg(i + 1);
            track.durationMs = (i + 1) * 61000;
            shelf.tracks.append(track);
        }
        discover->addShelf(shelf);
        pump(200);

        QListWidget *shelfList = discover->shelfList(DiscoverPage::deezerShelfId());
        check(shelfList && shelfList->count() == 5,
              QStringLiteral("полка Deezer заполнена переданными строками"));
        check(discover->shelfTrackCount(DiscoverPage::deezerShelfId()) == 5,
              QStringLiteral("строка-подсказка не считается треком чарта"));

        if (shelfList && shelfList->count() == 5) {
            const QListWidgetItem *row = shelfList->item(0);
            check(row->text() == QStringLiteral("Тестовый трек 1"),
                  QStringLiteral("название строки чарта"));
            check(row->data(ShelfDelegate::ArtistRole).toString() == QStringLiteral("Исполнитель 1"),
                  QStringLiteral("исполнитель строки чарта"));
            check(row->data(ShelfDelegate::DurationRole).toString() == QLatin1String("1:01"),
                  QStringLiteral("длительность строки в формате м:сс"));
            check(row->data(ShelfDelegate::SourceRole).toString() == QLatin1String("Deezer"),
                  QStringLiteral("источник подписан внутри строки"));

        }

        // Скриншот раздела: снимаем до клика по строке, а всплывающие сообщения
        // (они живут 6 секунд) успевают исчезнуть, чтобы не закрывать полки.
        pump(6500);
        nav->setCurrentRow(0);
        pump(200);
        const QString discoverShot
            = QDir(shotsDir).filePath(QStringLiteral("page-discover.png"));
        const bool saved = window.grab().save(discoverShot);
        check(saved, QStringLiteral("скриншот страницы «Обзор» сохранён"));
        if (saved)
            info(discoverShot);

        if (shelfList && shelfList->count() == 5) {
            // Двойной клик = «слушать полностью»: наружу уходит запрос трека.
            qRegisterMetaType<QListWidgetItem *>("QListWidgetItem*");
            ChartTrack requested;
            int requests = 0;
            QObject::connect(discover, &DiscoverPage::playRequested,
                             [&](const ChartTrack &track) {
                                 ++requests;
                                 requested = track;
                             });
            const bool invoked = QMetaObject::invokeMethod(
                shelfList, "itemDoubleClicked", Qt::DirectConnection,
                Q_ARG(QListWidgetItem *, shelfList->item(0)));
            pump(80);
            check(invoked, QStringLiteral("двойной клик по строке чарта обработан"));
            check(requests == 1, QStringLiteral("наружу ушёл запрос на прослушивание"));
            check(requested.searchQuery() == QStringLiteral("Исполнитель 1 - Тестовый трек 1"),
                  QStringLiteral("запрос для yt-dlp: «исполнитель — название»"));
            check(requested.previewUrl.endsWith(QLatin1String("preview1.mp3")),
                  QStringLiteral("превью трека доходит до плеера"));
        }

        // Сообщение о поиске висит 6 секунд — ждём, чтобы не мешало дальше.
        pump(6200);
    }

    // --- свой CSS и аварийный выход ----------------------------------------
    std::printf("\nСвой CSS:\n");
    // Картинки страниц оформления и YouTube: их стоит увидеть глазами.
    themes.setCurrentTheme(QStringLiteral("liquid-glass"));
    {
        const QStringList shots = {makeWav(dir.path(), QStringLiteral("Arrival.wav")),
                                   makeWav(dir.path(), QStringLiteral("Late Night Mix.wav")),
                                   makeWav(dir.path(), QStringLiteral("Rain on Glass.wav"))};
        QMetaObject::invokeMethod(library, "filesDropped", Q_ARG(QStringList, shots));
        playlist->setCurrentIndex(1);
        nav->setCurrentRow(3);            // Оформление
        pump(200);
        check(window.grab().save(QDir(shotsDir).filePath(QStringLiteral("page-appearance.png"))),
              QStringLiteral("скриншот страницы «Оформление» сохранён"));

        nav->setCurrentRow(2);            // YouTube
        pump(200);
        check(window.grab().save(QDir(shotsDir).filePath(QStringLiteral("page-youtube.png"))),
              QStringLiteral("скриншот страницы «YouTube» сохранён"));

        // Низ страницы оформления: фон, шрифт, стекло и редактор своего CSS.
        nav->setCurrentRow(3);
        pump(150);
        if (auto *scroll = window.findChild<QScrollArea *>()) {
            scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
            pump(150);
            check(window.grab().save(QDir(shotsDir).filePath(
                      QStringLiteral("page-appearance-bottom.png"))),
                  QStringLiteral("скриншот низа страницы «Оформление» сохранён"));
        }

        nav->setCurrentRow(0);
        pump(80);
    }
    themes.setCustomCss(QStringLiteral("QLabel[role=\"title\"] { color: @accent; }\n"
                                       "@import url('http://example.com/x.css');\n"));
    pump(80);
    check(!themes.lastProblems().isEmpty(), QStringLiteral("запрещённый @import попал в отчёт"));
    check(app.styleSheet().contains(QStringLiteral("QLabel[role=\"title\"] { color:")),
          QStringLiteral("свой CSS применён"));
    check(!app.styleSheet().contains(QLatin1String("@import")),
          QStringLiteral("@import не дошёл до QSS"));

    const QString withCustom = app.styleSheet();
    themes.toggleEscapeHatch();
    pump(60);
    check(app.styleSheet() != withCustom, QStringLiteral("аварийный Ctrl+Alt+Shift+C меняет оформление"));
    check(!app.styleSheet().contains(QStringLiteral("/* --- свой CSS --- */")),
          QStringLiteral("в безопасном виде свой CSS не применяется"));
    themes.toggleEscapeHatch();
    pump(60);
    check(app.styleSheet().contains(QStringLiteral("/* --- свой CSS --- */")),
          QStringLiteral("повторное нажатие возвращает свой CSS"));

    // --- сброс оформления ---------------------------------------------------
    themes.resetAppearance();
    pump(60);
    check(themes.currentThemeId() == themes.defaultThemeId(),
          QStringLiteral("после сброса активна тема по умолчанию"));
    check(themes.lastProblems().isEmpty(), QStringLiteral("после сброса замечаний нет"));

    // Обычная рамка окна тоже должна работать.
    AppearancePage::setFramelessEnabled(false);
    {
        MainWindow normal;
        normal.resize(1000, 700);
        normal.show();
        pump(120);
        check(!normal.windowFlags().testFlag(Qt::FramelessWindowHint),
              QStringLiteral("с выключенной «своей шапкой» окно использует системную рамку"));
        check(normal.findChild<PlayerBar *>(QStringLiteral("playerBar")) != nullptr,
              QStringLiteral("каркас собирается и в обычном режиме"));
    }

    std::printf("\n%s\n", g_failures == 0 ? "ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ" : "ЕСТЬ ПРОВАЛЫ");
    std::printf("Провалено: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}