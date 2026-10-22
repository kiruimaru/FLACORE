// Автотест обёртки YtDlp: запускает заглушку yt-dlp (tests/fake_ytdlp.cpp)
// и проверяет разбор JSON, прогресс, путь к файлу, ошибки и отмену.
// Сеть не нужна, YouTube не требуется.
#include "core/ytdlp/YtDlp.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>

#ifndef FAKE_YTDLP_PATH
#error "FAKE_YTDLP_PATH не задан (см. tests/CMakeLists.txt)"
#endif

namespace {

int g_failures = 0;

void check(bool condition, const char *what, int line)
{
    if (condition) {
        std::printf("  ok   %s\n", what);
    } else {
        std::printf("  FAIL %s (строка %d)\n", what, line);
        ++g_failures;
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

template <typename Predicate>
bool waitFor(Predicate predicate, int timeoutMs = 20000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }
    return predicate();
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FLACORE"));
    QCoreApplication::setApplicationName(QStringLiteral("ytdlp-selftest"));
    QStandardPaths::setTestModeEnabled(true);

    std::printf("YtDlp self-test\n");

    // --- 1. Проверка доступности -------------------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        bool done = false;
        bool available = false;
        QString info;

        QObject::connect(&ytdlp, &YtDlp::availabilityChecked,
                         [&](bool ok, const QString &payload) {
                             done = true;
                             available = ok;
                             info = payload;
                         });

        ytdlp.checkAvailability();
        CHECK(waitFor([&]() { return done; }));
        CHECK(available);
        CHECK(info == QLatin1String("2026.08.19-fake"));
        CHECK(!ytdlp.isBusy());
    }

    // --- 2. Поиск: разбор playlist/entries --------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QList<YtDlpResult> results;
        bool failed = false;

        QObject::connect(&ytdlp, &YtDlp::searchFinished,
                         [&](const QList<YtDlpResult> &list) { results = list; });
        QObject::connect(&ytdlp, &YtDlp::searchFailed, [&](const QString &) { failed = true; });

        ytdlp.search(QStringLiteral("fake query"), 5);
        CHECK(waitFor([&]() { return !results.isEmpty() || failed; }));
        CHECK(!failed);
        CHECK(results.size() == 2);

        if (results.size() == 2) {
            CHECK(results.at(0).title == QLatin1String("Fake Track One"));
            CHECK(results.at(0).uploader == QLatin1String("Fake Artist"));
            CHECK(results.at(0).durationSec == 245);
            CHECK(results.at(0).webpageUrl == QLatin1String("https://www.youtube.com/watch?v=aaa111"));

            // uploader отсутствует -> берётся channel; is_live -> длительность 0
            CHECK(results.at(1).uploader == QLatin1String("Fake Channel"));
            CHECK(results.at(1).durationSec == 0);
            CHECK(results.at(1).webpageUrl == QLatin1String("https://www.youtube.com/watch?v=bbb222"));
        }
    }

    // --- 2b. SoundCloud: префикс scsearch --------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QList<YtDlpResult> results;
        QObject::connect(&ytdlp, &YtDlp::searchFinished,
                         [&](const QList<YtDlpResult> &list) { results = list; });

        ytdlp.search(QStringLiteral("daft punk"), 5, YtDlp::Provider::SoundCloud);
        CHECK(waitFor([&]() { return !results.isEmpty(); }));
        CHECK(results.size() == 1);
        if (results.size() == 1)
            CHECK(results.at(0).title == QLatin1String("SC Search Hit"));
    }

    // --- 3. Прямой URL: одиночный объект без entries ----------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QList<YtDlpResult> results;
        QObject::connect(&ytdlp, &YtDlp::searchFinished,
                         [&](const QList<YtDlpResult> &list) { results = list; });

        ytdlp.search(QStringLiteral("https://soundcloud.com/example/direct-url-track"));
        CHECK(waitFor([&]() { return !results.isEmpty(); }));
        CHECK(results.size() == 1);
        if (results.size() == 1) {
            CHECK(results.at(0).title == QLatin1String("Direct Url Track"));
            CHECK(results.at(0).durationSec == 61);
            CHECK(results.at(0).webpageUrl
                  == QLatin1String("https://soundcloud.com/example/direct-url-track"));
        }
    }

    // --- 4. Скачивание: прогресс и путь к файлу ---------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QTemporaryDir dir;
        CHECK(dir.isValid());

        QString finishedPath;
        QString error;
        QList<int> percents;

        QObject::connect(&ytdlp, &YtDlp::downloadProgress,
                         [&](int percent, const QString &, const QString &, qint64, qint64) {
                             percents.append(percent);
                         });
        QObject::connect(&ytdlp, &YtDlp::downloadFinished,
                         [&](const QString &path) { finishedPath = path; });
        QObject::connect(&ytdlp, &YtDlp::downloadFailed,
                         [&](const QString &message) { error = message; });

        ytdlp.download(QStringLiteral("https://example.test/track"), dir.path());
        CHECK(waitFor([&]() { return !finishedPath.isEmpty() || !error.isEmpty(); }));

        CHECK(error.isEmpty());
        CHECK(!percents.isEmpty());
        CHECK(percents.value(percents.size() - 1) == 100);
        CHECK(finishedPath.contains(QLatin1String("Fake Track [fakeid].m4a")));
        CHECK(QFileInfo::exists(finishedPath));
        // -o должен указывать внутрь запрошенного каталога
        CHECK(QFileInfo(finishedPath).absolutePath() == QDir(dir.path()).absolutePath());
    }

    // --- 5. Ошибка yt-dlp -------------------------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QTemporaryDir dir;
        QStringList finishedPaths;
        QObject::connect(&ytdlp, &YtDlp::downloadFilesFinished,
                         [&](const QStringList &paths) { finishedPaths = paths; });
        ytdlp.download(QStringLiteral("https://example.test/playlist"),
                       dir.path(), QString(), true, true);
        CHECK(waitFor([&]() { return !finishedPaths.isEmpty(); }));
        CHECK(finishedPaths.size() == 2);
        if (finishedPaths.size() == 2) {
            CHECK(QFileInfo::exists(finishedPaths.at(0)));
            CHECK(QFileInfo::exists(finishedPaths.at(1)));
        }
    }

    // --- 6. Ошибка yt-dlp -------------------------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        QString error;
        QObject::connect(&ytdlp, &YtDlp::downloadFailed,
                         [&](const QString &message) { error = message; });

        QTemporaryDir dir;
        ytdlp.download(QStringLiteral("https://fail.test/track"), dir.path());
        CHECK(waitFor([&]() { return !error.isEmpty(); }));
        CHECK(error.contains(QLatin1String("404")));
        CHECK(!ytdlp.isBusy());
    }

    // --- 7. Отмена --------------------------------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral(FAKE_YTDLP_PATH));

        bool cancelled = false;
        QString error;
        QObject::connect(&ytdlp, &YtDlp::cancelled, [&]() { cancelled = true; });
        QObject::connect(&ytdlp, &YtDlp::downloadFailed,
                         [&](const QString &message) { error = message; });

        QTemporaryDir dir;
        ytdlp.download(QStringLiteral("https://slow.test/track"), dir.path());
        CHECK(waitFor([&]() { return ytdlp.mode() == YtDlp::Mode::Downloading; }, 5000));
        QThread::msleep(300);
        ytdlp.cancel();
        CHECK(waitFor([&]() { return cancelled || !error.isEmpty(); }));
        CHECK(cancelled);
        CHECK(error.isEmpty()); // отмена не должна выглядеть как ошибка
        CHECK(!ytdlp.isBusy());
    }

    // --- 8. Отсутствующий бинарник ---------------------------------------
    {
        YtDlp ytdlp;
        ytdlp.setBinary(QStringLiteral("definitely-not-a-binary-xyz"));

        bool available = true;
        QString message;
        QObject::connect(&ytdlp, &YtDlp::availabilityChecked,
                         [&](bool ok, const QString &info) { available = ok; message = info; });

        ytdlp.checkAvailability();
        CHECK(waitFor([&]() { return !message.isEmpty(); }));
        CHECK(!available);
        CHECK(message.contains(QLatin1String("yt-dlp"), Qt::CaseInsensitive));
    }

    if (g_failures == 0)
        std::printf("\nВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    else
        std::printf("\nПРОВАЛЕНО: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
