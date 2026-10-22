// Заглушка yt-dlp для автотестов: повторяет только тот контракт CLI,
// на который опирается YtDlp (аргументы, JSON, строки прогресса).
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

const char *kSearchJson = R"({
  "_type": "playlist",
  "entries": [
    {
      "id": "aaa111",
      "title": "Fake Track One",
      "duration": 245.4,
      "uploader": "Fake Artist",
      "url": "https://www.youtube.com/watch?v=aaa111",
      "is_live": false
    },
    {
      "id": "bbb222",
      "title": "Fake Track Two",
      "duration": null,
      "channel": "Fake Channel",
      "url": "https://www.youtube.com/watch?v=bbb222",
      "is_live": true
    }
  ]
})";

const char *kSingleJson = R"({
  "id": "ccc333",
  "title": "Direct Url Track",
  "duration": 61.0,
  "uploader": "Direct Artist",
  "webpage_url": "https://soundcloud.com/example/direct-url-track"
})";

const char *kScSearchJson = R"({
  "id": "sc1",
  "title": "SC Search Hit",
  "duration": 120.0,
  "uploader": "SC Artist",
  "webpage_url": "https://soundcloud.com/sc/hit"
})";

bool hasArg(int argc, char **argv, const char *needle)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strstr(argv[i], needle))
            return true;
    }
    return false;
}

std::string argValue(int argc, char **argv, const char *flag)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0)
            return argv[i + 1];
    }
    return {};
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "ERROR: no arguments\n");
        return 2;
    }

    if (std::strcmp(argv[1], "--version") == 0) {
        std::printf("2026.08.19-fake\n");
        return 0;
    }

    if (hasArg(argc, argv, "scsearch")) {
        std::printf("%s\n", kScSearchJson);
        return 0;
    }

    if (hasArg(argc, argv, "ytsearch")) {
        std::printf("%s\n", kSearchJson);
        return 0;
    }

    if (hasArg(argc, argv, "fail.test")) {
        std::fprintf(stderr, "ERROR: [generic] fail.test: HTTP Error 404: Not Found\n");
        return 1;
    }

    const bool downloading = !argValue(argc, argv, "-o").empty();

    if (!downloading) {
        std::printf("%s\n", kSingleJson);
        return 0;
    }

    std::string outTemplate = argValue(argc, argv, "-o");
    std::string dir = outTemplate.substr(0, outTemplate.find_last_of("/\\"));
    if (dir.empty())
        dir = ".";

    const std::string finalPath = dir + "/Fake Track [fakeid].m4a";
    std::ofstream(finalPath, std::ios::binary) << "fake audio payload";

    const bool slow = hasArg(argc, argv, "slow.test");
    const int steps = slow ? 50 : 3;
    const auto emitProgress = [&](int percent) {
        std::printf("FLACORE_PROGRESS %d.0%%|%d|1000000| 1.00MiB/s|00:01\n", percent,
                    percent * 10000);
        std::fflush(stdout);
    };

    for (int i = 0; i <= steps; ++i) {
        emitProgress(static_cast<int>(100.0 * i / steps));
        if (slow)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::printf("FLACORE_FILE %s\n", finalPath.c_str());
    if (hasArg(argc, argv, "--yes-playlist")) {
        const std::string secondPath = dir + "/Fake Track Two [fakeid2].m4a";
        std::ofstream(secondPath, std::ios::binary) << "fake audio payload";
        std::printf("FLACORE_FILE %s\n", secondPath.c_str());
    }
    std::fflush(stdout);
    return 0;
}
