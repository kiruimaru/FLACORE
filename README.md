<div align="center">

  <!-- Неоновый киберпанк-баннер -->
  <img src="https://capsule-render.vercel.app/api?type=waving&height=250&color=0:141E30,100:243B55&text=FLACORE&fontSize=75&fontColor=64ffda&animation=fadeIn&desc=local%20music%20player%20%2B%20yt-dlp&descSize=18&descAlignY=70" />

  <p>
    <img src="https://img.shields.io/badge/license-GPL--3.0-blue.svg" alt="License">
    <img src="https://img.shields.io/badge/C%2B%2B-17-00599c.svg" alt="C++17">
    <img src="https://img.shields.io/badge/Qt-6.5%2B-41cd52.svg" alt="Qt 6.5+">
    <img src="https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20FreeBSD-lightgrey.svg" alt="Platforms">
  </p>

  <p>
    <b>English</b> · <a href="#русский">Русский</a>
  </p>

</div>

# FLACORE

A local music player written in C++17 / Qt 6. It plays your own library and pulls
music from YouTube, SoundCloud and 1000+ other sites through **yt-dlp** — search,
listen immediately, or save to a FLAC library, all inside the same window.

**Status:** 0.3.0 — early, actively breaking. Read [Known limitations](#known-limitations)
before expecting it to behave like foobar2000.

## Quick start

Clone FLACORE, build it, and add it to your applications menu:

```bash
sudo pacman -S --needed base-devel cmake desktop-file-utils qt6-base \
                       qt6-multimedia qt6-multimedia-ffmpeg yt-dlp ffmpeg

git clone https://github.com/kiruimaru/FLACORE.git
cd FLACORE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j"$(nproc)"
cmake --install build
update-desktop-database "$HOME/.local/share/applications"
```

Press `Super+Alt+Space`, search for **FLACORE**, and press Enter. In Omarchy,
`Super+Space` opens the main menu; `Super+Alt+Space` opens the Apps menu.

- **Play local music:** choose `File → Open Files`, then double-click a playlist
  track. The next track starts automatically when it finishes.
- **Search online:** open the `YouTube` tab, search for a track, then choose
  **Listen** to play it or **To library** to save it.
- **Launch from a terminal:** run `flacore`.

`yt-dlp` is needed for online search and downloads. `ffmpeg` is used when saving
audio in a selected format. For other Linux distributions and Windows, see
[Build](#build).

---

## Table of contents

- [Quick start](#quick-start)
- [Features](#features)
- [Screenshots](#screenshots)
- [Requirements](#requirements)
- [Build](#build)
- [Usage](#usage)
- [How the yt-dlp integration works](#how-the-yt-dlp-integration-works)
- [Configuration](#configuration)
- [Tests](#tests)
- [Project structure](#project-structure)
- [Known limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Legal](#legal)
- [License](#license)
- [Русская версия](#русский)

---

## Features

### Local playback

- FLAC, MP3, WAV, OGG, M4A, AAC through Qt Multimedia
- Multi-file open dialog, playlist in the right panel
- Play / pause / stop / next / previous, automatic next-track playback,
  drag-to-seek, volume
- Double-click a playlist row to play it

### Online (yt-dlp)

- **Search** on YouTube or SoundCloud from the `YouTube` tab (15 results)
- **Paste a link** and get the track without searching — URL detection is automatic
- **Listen** — downloads the best audio stream into a local cache and starts
  playback; a second click reuses the cached file
- **Save to library** — extracts audio to FLAC (default), MP3, Opus or M4A,
  embeds metadata, and drops the track into the playlist
- Live download progress (percent, speed, ETA), a cancel button, and readable
  errors instead of silent failure

### Under the hood

- `YtDlp` is an async `QProcess` wrapper — no shell, no injection, no blocking UI
- One process at a time; the UI disables itself while a job is running
- No FFmpeg **libraries** required to build; the `yt-dlp` and `ffmpeg`
  executables are called at runtime

---

## Screenshots

> No screenshots in the repository yet. Add them here (drag & drop into the
> GitHub editor uploads the file and inserts a link):

```
docs/screenshot-main.png      — main window: player + playlist
docs/screenshot-search.png    — YouTube tab with search results
docs/screenshot-settings.png  — yt-dlp settings dialog
```

---

## Requirements

| What | Why | Version |
|------|-----|---------|
| C++ compiler | build | C++17 (GCC 11+, Clang 14+, MSVC 2019+) |
| CMake | build | 3.21+ |
| Qt 6: `Core`, `Gui`, `Widgets`, `Multimedia` | UI + playback | 6.5+ (tested on 6.12) |
| `yt-dlp` | online search / download | recent (tested 2026.08.19) |
| `ffmpeg` binary | **only** for saving as FLAC/MP3/Opus | 4.x+ |
| Qt Multimedia backend | actual audio output | `qt6-multimedia-ffmpeg` on Linux |

`ffmpeg` is optional: listening works without it because the stream is cached
as-is. Saving to FLAC/MP3/Opus needs it, since that is a re-encode.

FFmpeg **dev** packages (`libavcodec-dev` etc.) are **not** needed. If you want
them anyway, configure with `-DFLACORE_USE_FFMPEG_LIBS=ON`.

---

## Build

### Debian / Ubuntu

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev \
                 qt6-multimedia-ffmpeg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

### Arch Linux

```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-multimedia
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

### Fedora

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

### Windows — MSYS2 (recommended, verified)

Install [MSYS2](https://www.msys2.org/) and open the **UCRT64** shell:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-ninja \
                     mingw-w64-ucrt-x86_64-qt6-base \
                     mingw-w64-ucrt-x86_64-qt6-multimedia

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/ucrt64
cmake --build build -j 8
./build/src/flacore.exe
```

Running the `.exe` outside the MSYS2 shell needs the Qt DLLs nearby — either add
`C:\msys64\ucrt64\bin` to `PATH` or run `windeployqt build/src/flacore.exe`.

### Windows — MSVC

Requires Qt 6 for MSVC (online installer) **and** the Windows 10/11 SDK. Build
Tools alone are not enough: without the SDK, `cl.exe` cannot compile anything.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --config Release
```

### FreeBSD / other Unices

The CMake files are portable; install `qt6-base`, `qt6-multimedia`, `cmake` and
`ninja` from ports/packages. This configuration is not tested by the maintainers.

### Install `yt-dlp` and `ffmpeg`

```bash
# yt-dlp
pipx install yt-dlp                # or: pip install --user -U yt-dlp
sudo pacman -S yt-dlp              # Arch
winget install yt-dlp.yt-dlp       # Windows

# ffmpeg (only for FLAC/MP3/Opus export)
sudo apt install ffmpeg            # Debian/Ubuntu
winget install Gyan.FFmpeg         # Windows
```

If `yt-dlp` is not in `PATH`, point the app at it:
`Tools → yt-dlp settings…` (Инструменты → Настройки yt-dlp…).

---

## Usage

### Play local files

1. `File → Open Files` (`Ctrl+O`) and pick one or more audio files.
2. Double-click a row in the `Playlist` tab, or press Play.
3. Drag the progress slider to seek; the volume slider sits below the buttons.

### Find and listen to something online

1. Switch to the `YouTube` tab.
2. Choose the source in the dropdown: **YouTube** or **SoundCloud**.
3. Type a query and press `Enter` / «Найти». Results show title, author, duration.
4. Pick a row and press:
   - **«Слушать»** — download into the cache and start playing right away;
   - **«В библиотеку»** — save to the library folder (FLAC by default) and add
     the track to the playlist.
5. `Отмена` stops the current download; `Ctrl+Q` quits the app.

Pasting a direct link (`https://…`) into the search field skips the search and
fetches that single track.

### Settings

`Tools → yt-dlp settings…` — path to `yt-dlp`, the folder containing `ffmpeg`,
the library folder, and the save format (FLAC / MP3 320 / Opus / M4A). The dialog
probes `yt-dlp --version` and shows the result immediately. `Tools → Open library
folder` opens the library in your file manager.

---

## How the yt-dlp integration works

`src/core/ytdlp/YtDlp.{h,cpp}` is a thin, opinionated wrapper around the CLI.
These are the exact commands it builds:

**Search**

```
yt-dlp "ytsearch15:<query>" --dump-single-json --flat-playlist --skip-download --no-warnings
yt-dlp "scsearch15:<query>" --dump-single-json --flat-playlist --skip-download --no-warnings
```

**Single URL**

```
yt-dlp "<url>" --no-playlist --dump-single-json --flat-playlist --skip-download --no-warnings
```

**Listen (cache, no re-encode)**

```
yt-dlp -f "bestaudio[ext=m4a]/bestaudio/best" \
       --no-playlist --newline --no-warnings --progress \
       --progress-template "download:FLACORE_PROGRESS %(progress._percent_str)s|%(progress.downloaded_bytes)s|%(progress.total_bytes_estimate)s|%(progress._speed_str)s|%(progress._eta_str)s" \
       --print "after_move:FLACORE_FILE %(filepath)s" \
       -o "<cache>/%(title)s [%(id)s].%(ext)s" "<url>"
```

**Save to library**

```
yt-dlp -f bestaudio/best -x --audio-format flac --audio-quality 0 \
       --embed-metadata --add-metadata --ffmpeg-location "<ffmpeg dir>" \
       --no-playlist --newline --no-warnings --progress \
       --progress-template "download:FLACORE_PROGRESS ..." \
       --print "after_move:FLACORE_FILE %(filepath)s" \
       -o "<library>/%(artist,uploader,channel)s - %(title)s.%(ext)s" "<url>"
```

Why this shape:

- `m4a` is preferred for playback because the WMF backend on Windows cannot play
  WebM/Opus, while MP3/M4A/FLAC work everywhere.
- `--print after_move:filepath` is the only reliable way to learn the final path
  after post-processing (`-x` changes the extension and may rename the file).
- `--progress` is passed explicitly: `--print` implies `--quiet`, which would
  otherwise swallow the progress lines.
- `PYTHONIOENCODING=utf-8` and `PYTHONUTF8=1` are set for the child process so
  Cyrillic titles are not mangled on Windows.

---

## Configuration

Stored with `QSettings` — no config file parsing involved:

| Key | Meaning | Default |
|-----|---------|---------|
| `ytdlp/binary` | full path to `yt-dlp` | auto: `PATH` |
| `ytdlp/ffmpegDir` | folder containing the `ffmpeg` binary | auto: `PATH` |
| `ytdlp/libraryDir` | where «В библиотеку» saves | `~/Music/FLACORE` |
| `ytdlp/audioFormat` | `flac` / `mp3` / `opus` / `m4a` | `flac` |

Where it lands:

- Linux: `~/.config/flacore.conf`, cache in `~/.cache/flacore/ytdlp`
- Windows: registry `HKEY_CURRENT_USER\Software\flacore`, cache in
  `%LOCALAPPDATA%\flacore\cache\ytdlp`

Delete those to factory-reset the app.

---

## Tests

No network, no real `yt-dlp`, no audio device needed — the CLI contract is
exercised against a compiled stub (`tests/fake_ytdlp.cpp`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

What is covered (39 assertions):

- availability probe: `--version` output and the "binary missing" path
- search JSON: playlist with `entries`, single object for a direct URL,
  `uploader` → `channel` fallback, live streams → duration 0
- provider routing: `ytsearch` vs `scsearch`
- download: progress percentages, final path from `FLACORE_FILE`, file exists
- failures: non-zero exit with `ERROR: …` in stderr
- cancel: process killed → `cancelled()`, no error signal

Turn tests off with `-DFLACORE_BUILD_TESTS=OFF`.

---

## Project structure

```
FLACORE/
├── CMakeLists.txt                # Qt6 lookup, optional FFmpeg libs, tests
├── src/
│   ├── main.cpp                  # QApplication + MainWindow
│   ├── CMakeLists.txt            # qt_add_executable, /W3 or -Wall -Wextra
│   ├── core/
│   │   ├── player/Playlist.*     # Track struct, list, current index, signals
│   │   └── ytdlp/
│   │       ├── YtDlpTypes.h      # YtDlpResult, formatDuration()
│   │       └── YtDlp.*           # QProcess wrapper: search/download/cancel
│   └── ui/
│       ├── MainWindow.*          # transport, playlist, tabs, menus
│       ├── YouTubePanel.*        # search UI, progress, play/save buttons
│       └── YtDlpSettingsDialog.* # binary paths, library, format
├── packaging/
│   └── linux/flacore.desktop    # Linux applications-menu entry
├── tests/
│   ├── fake_ytdlp.cpp            # stub with the same CLI contract
│   ├── ytdlp_test.cpp            # 39 assertions, no network
│   └── CMakeLists.txt
├── LICENSE                       # GPL-3.0
└── README.md
```

Layering rule: `core/` knows nothing about widgets, `ui/` never spawns processes
directly. `YouTubePanel` emits `playRequested(path)` / `fileAdded(path)` and
`MainWindow` decides what to do with the file.

---

## Known limitations

An honest list — these are real, not hypothetical:

- **No tag reading.** Artist, cover and duration from files are ignored; the
  playlist shows the file name. `Track::artist` is always `"Unknown"`.
- **Playlist is in memory.** No M3U import/export, no persistence between runs.
- **No shuffle / repeat / queue.**
- **Removing a track above the current one** shifts the model index without
  notifying the view — the selection can point at the wrong row afterwards.
- **No media keys, no MPRIS, no tray icon.**
- **No bundled packages yet.** Linux installs include a `.desktop` launcher,
  but there is no AppImage, Flatpak, or Windows installer.
- **YouTube may be unreachable** from your network (it is from Russia without a
  proxy). Use the SoundCloud source, or add `--proxy` to `yt-dlp`'s own config.

---

## Roadmap

- [ ] `PlaylistModel : QAbstractListModel` + `QListView` (drag & drop, roles)
- [ ] tag and cover reading (libavformat or taglib), real library scanning
- [ ] M3U save/load, shuffle, repeat
- [ ] download queue instead of one job at a time
- [ ] MPRIS2 + media keys, tray icon
- [ ] GitHub Actions: build on Linux + tests
- [ ] packaging: AppImage / Flatpak / Windows installer

---

## Troubleshooting

**«yt-dlp не найден»** — the binary is not in `PATH`. Set the full path in
`Tools → yt-dlp settings…`. On Windows `pip install` puts it into
`%LOCALAPPDATA%\Programs\Python\Python3xx\Scripts`, which is often not in `PATH`.

**Download works, FLAC save fails** — no `ffmpeg`. Install it or point to its
folder in the settings dialog. The error text from `yt-dlp` appears under the
progress bar, usually `ffmpeg not found`.

**«ERROR: Unable to download API page … [WinError 10054]»** — the site is blocked
or reset by your ISP. YouTube is not reachable from some regions; use SoundCloud,
or configure a proxy in `~/.config/yt-dlp/config` (`--proxy http://host:port`).

**MP3 plays, WebM/Opus stays silent** — the Qt Multimedia backend cannot decode
it. On Linux install `qt6-multimedia-ffmpeg`; on Windows prefer the `m4a` source
(the app already asks for `bestaudio[ext=m4a]` first).

**`Could not find a package configuration file provided by "Qt6"`** — pass
`-DCMAKE_PREFIX_PATH=<Qt dir>`, e.g. `/ucrt64`, `C:/Qt/6.8.3/msvc2022_64`, or the
parent of `/usr/lib/cmake/Qt6`. With MSYS2 the value is `/ucrt64` inside the
MSYS2 shell.

**`CXX compiler identification is unknown` on Windows** — Visual Studio without
the Windows SDK, or no compiler at all. Use the MSYS2 route above.

**Progress stays at 0 %** — `yt-dlp` did not report `total_bytes_estimate`
(common for HLS). The bar stays in busy mode and the final path still arrives;
that is expected.

---

## Contributing

1. Build and run the tests before opening a PR (`ctest --test-dir build`).
2. Keep the layering (`core/` ↔ `ui/`) and put new logic in `core/` so it can be
   tested without a GUI.
3. UI strings may be Russian, but keep identifiers and comments consistent with
   the file you touch.

Bug reports are welcome with: OS, Qt version, `yt-dlp --version`, the exact
command from the log, and what you expected.

---

## Legal

FLACORE is a player. It ships no content and hosts nothing. The yt-dlp
integration is a convenience wrapper around a third-party tool: you are
responsible for what you download, for complying with the terms of service of the
sites you use, and with the copyright law of your country. Do not use it to
redistribute copyrighted material.

---

## License

GPL-3.0 — see [LICENSE](LICENSE).

---

<div align="center">
  <a href="#flacore">English</a> · <b>Русский</b>
</div>

## Русский

Локальный музыкальный плеер на C++17 / Qt 6. Играет вашу фонотеку и тянет музыку
с YouTube, SoundCloud и ещё 1000+ сайтов через **yt-dlp**: поиск, мгновенное
прослушивание или сохранение в библиотеку FLAC — всё в одном окне.

**Статус:** 0.3.0 — ранняя версия, всё ещё ломается. Перед тем как ждать
поведения foobar2000, прочитайте [Ограничения](#ограничения).

### Быстрый старт

Склонируйте FLACORE, соберите его и добавьте в меню приложений:

```bash
sudo pacman -S --needed base-devel cmake desktop-file-utils qt6-base \
                       qt6-multimedia qt6-multimedia-ffmpeg yt-dlp ffmpeg

git clone https://github.com/kiruimaru/FLACORE.git
cd FLACORE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j"$(nproc)"
cmake --install build
update-desktop-database "$HOME/.local/share/applications"
```

Нажмите `Super+Alt+Space`, найдите **FLACORE** и нажмите Enter. В Omarchy
сочетание `Super+Space` открывает главное меню, а `Super+Alt+Space` — меню
приложений.

- **Локальная музыка:** выберите `Файл → Открыть файлы`, затем дважды нажмите
  на трек в плейлисте. После его окончания автоматически начнётся следующий.
- **Поиск музыки онлайн:** откройте вкладку `YouTube`, найдите трек и нажмите
  **Слушать**, чтобы включить его, или **В библиотеку**, чтобы сохранить.
- **Запуск из терминала:** выполните `flacore`.

`yt-dlp` нужен для поиска и скачивания музыки онлайн. `ffmpeg` используется
при сохранении аудио в выбранном формате. Сборку в других Linux-дистрибутивах
и Windows смотрите в разделе [Сборка](#сборка).

### Содержание

- [Быстрый старт](#быстрый-старт)
- [Возможности](#возможности)
- [Требования](#требования)
- [Сборка](#сборка)
- [Использование](#использование)
- [Как работает интеграция с yt-dlp](#как-работает-интеграция-с-yt-dlp)
- [Настройки](#настройки)
- [Тесты](#тесты)
- [Структура проекта](#структура-проекта)
- [Ограничения](#ограничения)
- [Планы](#планы)
- [Решение проблем](#решение-проблем)
- [Лицензия](#лицензия)

### Возможности

**Локальное воспроизведение**

- FLAC, MP3, WAV, OGG, M4A, AAC через Qt Multimedia
- Открытие сразу нескольких файлов, плейлист в правой панели
- Play / пауза / стоп / следующий / предыдущий, автоматический переход к
  следующему треку, перемотка перетаскиванием, громкость
- Двойной клик по строке плейлиста запускает трек

**Онлайн (yt-dlp)**

- **Поиск** по YouTube или SoundCloud на вкладке `YouTube` (15 результатов)
- **Вставка ссылки** — трек без поиска, URL распознаётся автоматически
- **«Слушать»** — лучший аудиопоток скачивается в локальный кэш и сразу играет;
  повторное нажатие переиспользует уже скачанный файл
- **«В библиотеку»** — извлечение аудио в FLAC (по умолчанию), MP3, Opus или M4A
  с вшиванием метаданных; трек сразу попадает в плейлист
- Живой прогресс (проценты, скорость, ETA), кнопка отмены и понятные ошибки
  вместо молчаливого отказа

**Под капотом**

- `YtDlp` — асинхронная обёртка над `QProcess`: без shell, без инъекций,
  интерфейс не блокируется
- Один процесс за раз; на время работы элементы управления отключаются
- Библиотеки FFmpeg для сборки **не нужны** — вызываются исполняемые файлы
  `yt-dlp` и `ffmpeg`

### Скриншоты

> В репозитории пока нет скриншотов. Добавьте их сюда (перетаскивание картинки
> в редактор GitHub загружает файл и вставляет ссылку):

```
docs/screenshot-main.png      — главное окно: плеер + плейлист
docs/screenshot-search.png    — вкладка YouTube с результатами поиска
docs/screenshot-settings.png  — диалог настроек yt-dlp
```

### Требования

| Что | Зачем | Версия |
|-----|-------|--------|
| Компилятор C++ | сборка | C++17 (GCC 11+, Clang 14+, MSVC 2019+) |
| CMake | сборка | 3.21+ |
| Qt 6: `Core`, `Gui`, `Widgets`, `Multimedia` | интерфейс и воспроизведение | 6.5+ (проверено на 6.12) |
| `yt-dlp` | поиск и скачивание | свежий (проверено 2026.08.19) |
| бинарник `ffmpeg` | **только** для сохранения в FLAC/MP3/Opus | 4.x+ |
| бэкенд Qt Multimedia | собственно звук | `qt6-multimedia-ffmpeg` на Linux |

`ffmpeg` необязателен: прослушивание работает без него, потому что поток
кэшируется как есть. Сохранение в FLAC/MP3/Opus — это перекодирование, ему
`ffmpeg` нужен.

Пакеты FFmpeg **для разработки** (`libavcodec-dev` и т.п.) не требуются. Если
всё же нужны — конфигурируйте с `-DFLACORE_USE_FFMPEG_LIBS=ON`.

### Сборка

**Debian / Ubuntu**

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev \
                 qt6-multimedia-ffmpeg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

**Arch Linux**

```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-multimedia
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

**Fedora**

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/src/flacore
```

**Windows — MSYS2 (рекомендуется, проверено)**

Установите [MSYS2](https://www.msys2.org/) и откройте оболочку **UCRT64**:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-ninja \
                     mingw-w64-ucrt-x86_64-qt6-base \
                     mingw-w64-ucrt-x86_64-qt6-multimedia

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/ucrt64
cmake --build build -j 8
./build/src/flacore.exe
```

Чтобы запускать `.exe` вне оболочки MSYS2, рядом должны быть DLL Qt: либо
добавьте `C:\msys64\ucrt64\bin` в `PATH`, либо выполните
`windeployqt build/src/flacore.exe`.

**Windows — MSVC**

Нужны Qt 6 для MSVC (онлайн-установщик) **и** Windows 10/11 SDK. Одних Build
Tools недостаточно: без SDK `cl.exe` не соберёт вообще ничего.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --config Release
```

**FreeBSD и другие Unix**

CMake-файлы портируемы: поставьте `qt6-base`, `qt6-multimedia`, `cmake` и `ninja`
из портов/пакетов. Мейнтейнеры эту конфигурацию не тестировали.

**Установка `yt-dlp` и `ffmpeg`**

```bash
# yt-dlp
pipx install yt-dlp                # или: pip install --user -U yt-dlp
sudo pacman -S yt-dlp              # Arch
winget install yt-dlp.yt-dlp       # Windows

# ffmpeg (только для экспорта в FLAC/MP3/Opus)
sudo apt install ffmpeg            # Debian/Ubuntu
winget install Gyan.FFmpeg         # Windows
```

Если `yt-dlp` не в `PATH`, укажите путь в приложении:
«Инструменты → Настройки yt-dlp…» (`Tools → yt-dlp settings…`).

### Использование

**Локальные файлы**

1. `File → Open Files` (`Ctrl+O`) — выберите один или несколько аудиофайлов.
2. Двойной клик по строке на вкладке `Playlist` или кнопка воспроизведения.
3. Перемотка — перетаскиванием ползунка; громкость — ползунком ниже кнопок.

**Поиск и прослушивание онлайн**

1. Перейдите на вкладку `YouTube`.
2. Выберите источник в выпадающем списке: **YouTube** или **SoundCloud**.
3. Введите запрос и нажмите `Enter` / «Найти». В результатах видны название,
   автор и длительность.
4. Выберите строку и нажмите:
   - **«Слушать»** — скачать в кэш и сразу включить;
   - **«В библиотеку»** — сохранить в папку библиотеки (по умолчанию FLAC)
     и добавить трек в плейлист.
5. «Отмена» прерывает текущее скачивание, `Ctrl+Q` закрывает приложение.

Если вставить в поле поиска прямую ссылку (`https://…`), поиск пропускается и
загружается именно этот трек.

**Настройки**

«Инструменты → Настройки yt-dlp…» — путь к `yt-dlp`, папка с `ffmpeg`, папка
библиотеки и формат сохранения (FLAC / MP3 320 / Opus / M4A). Диалог сразу
проверяет `yt-dlp --version` и показывает результат. «Инструменты → Открыть
папку библиотеки» открывает её в файловом менеджере.

### Как работает интеграция с yt-dlp

`src/core/ytdlp/YtDlp.{h,cpp}` — тонкая обёртка над CLI. Вот точные команды,
которые она собирает:

**Поиск**

```
yt-dlp "ytsearch15:<запрос>" --dump-single-json --flat-playlist --skip-download --no-warnings
yt-dlp "scsearch15:<запрос>" --dump-single-json --flat-playlist --skip-download --no-warnings
```

**Одна ссылка**

```
yt-dlp "<url>" --no-playlist --dump-single-json --flat-playlist --skip-download --no-warnings
```

**Прослушивание (кэш, без перекодирования)**

```
yt-dlp -f "bestaudio[ext=m4a]/bestaudio/best" \
       --no-playlist --newline --no-warnings --progress \
       --progress-template "download:FLACORE_PROGRESS %(progress._percent_str)s|%(progress.downloaded_bytes)s|%(progress.total_bytes_estimate)s|%(progress._speed_str)s|%(progress._eta_str)s" \
       --print "after_move:FLACORE_FILE %(filepath)s" \
       -o "<кэш>/%(title)s [%(id)s].%(ext)s" "<url>"
```

**Сохранение в библиотеку**

```
yt-dlp -f bestaudio/best -x --audio-format flac --audio-quality 0 \
       --embed-metadata --add-metadata --ffmpeg-location "<папка ffmpeg>" \
       --no-playlist --newline --no-warnings --progress \
       --progress-template "download:FLACORE_PROGRESS ..." \
       --print "after_move:FLACORE_FILE %(filepath)s" \
       -o "<библиотека>/%(artist,uploader,channel)s - %(title)s.%(ext)s" "<url>"
```

Почему именно так:

- `m4a` предпочтителен для проигрывания: WMF-бэкенд в Windows не умеет WebM/Opus,
  а MP3/M4A/FLAC играются везде.
- `--print after_move:filepath` — единственный надёжный способ узнать итоговый
  путь после постобработки (`-x` меняет расширение и может переименовать файл).
- `--progress` передаётся явно: `--print` включает `--quiet`, который иначе
  съел бы строки прогресса.
- Для дочернего процесса выставляются `PYTHONIOENCODING=utf-8` и `PYTHONUTF8=1`,
  иначе на Windows ломаются кириллические названия.

### Настройки

Хранятся через `QSettings`, своего парсера конфигов нет:

| Ключ | Значение | По умолчанию |
|------|----------|--------------|
| `ytdlp/binary` | полный путь к `yt-dlp` | авто: `PATH` |
| `ytdlp/ffmpegDir` | папка с бинарником `ffmpeg` | авто: `PATH` |
| `ytdlp/libraryDir` | куда сохраняет «В библиотеку» | `~/Music/FLACORE` |
| `ytdlp/audioFormat` | `flac` / `mp3` / `opus` / `m4a` | `flac` |

Где это лежит:

- Linux: `~/.config/flacore.conf`, кэш в `~/.cache/flacore/ytdlp`
- Windows: реестр `HKEY_CURRENT_USER\Software\flacore`, кэш в
  `%LOCALAPPDATA%\flacore\cache\ytdlp`

Удалите эти данные, чтобы сбросить приложение к состоянию «как после установки».

### Тесты

Сеть, настоящий `yt-dlp` и звуковое устройство не нужны — контракт CLI
проверяется на скомпилированной заглушке (`tests/fake_ytdlp.cpp`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Что покрыто (39 проверок):

- проверка доступности: вывод `--version` и путь «бинарник отсутствует»
- разбор JSON: плейлист с `entries`, одиночный объект для прямой ссылки,
  подстановка `uploader` → `channel`, live-стримы → длительность 0
- маршрутизация источника: `ytsearch` против `scsearch`
- скачивание: проценты прогресса, итоговый путь из `FLACORE_FILE`, файл существует
- ошибки: ненулевой код выхода и `ERROR: …` в stderr
- отмена: процесс убит → `cancelled()`, без сигнала ошибки

Отключить тесты: `-DFLACORE_BUILD_TESTS=OFF`.

### Структура проекта

```
FLACORE/
├── CMakeLists.txt                # поиск Qt6, опциональные libav*, тесты
├── src/
│   ├── main.cpp                  # QApplication + MainWindow
│   ├── CMakeLists.txt            # qt_add_executable, /W3 или -Wall -Wextra
│   ├── core/
│   │   ├── player/Playlist.*     # Track, список, текущий индекс, сигналы
│   │   └── ytdlp/
│   │       ├── YtDlpTypes.h      # YtDlpResult, formatDuration()
│   │       └── YtDlp.*           # QProcess: search/download/cancel
│   └── ui/
│       ├── MainWindow.*          # транспорт, плейлист, вкладки, меню
│       ├── YouTubePanel.*        # поиск, прогресс, кнопки «Слушать»/«В библиотеку»
│       └── YtDlpSettingsDialog.* # пути к бинарникам, библиотека, формат
├── packaging/
│   └── linux/flacore.desktop    # ярлык в меню приложений Linux
├── tests/
│   ├── fake_ytdlp.cpp            # заглушка с тем же CLI-контрактом
│   ├── ytdlp_test.cpp            # 39 проверок без сети
│   └── CMakeLists.txt
├── LICENSE                       # GPL-3.0
└── README.md
```

Правило слоёв: `core/` ничего не знает о виджетах, `ui/` никогда не запускает
процессы напрямую. `YouTubePanel` отдаёт наружу `playRequested(path)` /
`fileAdded(path)`, а `MainWindow` решает, что делать с файлом.

### Ограничения

Честный список — это реальные вещи, а не гипотетические:

- **Нет чтения тегов.** Артист, обложка и длительность из файлов игнорируются,
  в плейлисте — имя файла. `Track::artist` всегда `"Unknown"`.
- **Плейлист живёт в памяти.** Нет импорта/экспорта M3U, нет сохранения между
  запусками.
- **Нет shuffle / repeat / очереди.**
- **Удаление трека выше текущего** сдвигает индекс модели, не уведомляя
  представление — выделение может указывать не на ту строку.
- **Нет медиаклавиш, MPRIS и значка в трее.**
- **Пока нет готовых установочных пакетов.** При установке в Linux создаётся
  ярлык `.desktop`, но AppImage, Flatpak и установщика для Windows пока нет.
- **YouTube может быть недоступен** из вашей сети (например, из России без
  прокси). Используйте источник SoundCloud или добавьте `--proxy` в конфиг
  самого `yt-dlp`.

### Планы

- [ ] `PlaylistModel : QAbstractListModel` + `QListView` (drag & drop, роли)
- [ ] чтение тегов и обложек (libavformat или taglib), сканирование фонотеки
- [ ] сохранение/загрузка M3U, shuffle, repeat
- [ ] очередь скачивания вместо одной задачи
- [ ] MPRIS2 + медиаклавиши, значок в трее
- [ ] GitHub Actions: сборка на Linux + тесты
- [ ] упаковка: AppImage / Flatpak / установщик для Windows

### Решение проблем

**«yt-dlp не найден»** — бинарника нет в `PATH`. Укажите полный путь в
«Инструменты → Настройки yt-dlp…». На Windows `pip install` кладёт его в
`%LOCALAPPDATA%\Programs\Python\Python3xx\Scripts`, а этой папки часто нет в `PATH`.

**Скачивание идёт, сохранение в FLAC падает** — нет `ffmpeg`. Установите его или
укажите папку в настройках. Текст ошибки `yt-dlp` показывается под полосой
прогресса, обычно там `ffmpeg not found`.

**«ERROR: Unable to download API page … [WinError 10054]»** — сайт заблокирован
или соединение сбрасывает провайдер. YouTube недоступен из ряда регионов;
используйте SoundCloud или пропишите прокси в `~/.config/yt-dlp/config`
(`--proxy http://host:port`).

**MP3 играет, а WebM/Opus молчит** — бэкенд Qt Multimedia его не декодирует.
На Linux поставьте `qt6-multimedia-ffmpeg`; на Windows приложение и так
предпочитает `m4a` (`bestaudio[ext=m4a]`).

**`Could not find a package configuration file provided by "Qt6"`** — передайте
`-DCMAKE_PREFIX_PATH=<папка Qt>`, например `/ucrt64`,
`C:/Qt/6.8.3/msvc2022_64` или родителя `/usr/lib/cmake/Qt6`. В MSYS2 значение —
`/ucrt64` внутри оболочки MSYS2.

**`CXX compiler identification is unknown` на Windows** — Visual Studio без
Windows SDK либо компилятора нет вообще. Используйте путь через MSYS2.

**Прогресс стоит на 0 %** — `yt-dlp` не сообщил `total_bytes_estimate` (частое
дело для HLS). Полоса остаётся в режиме «занято», итоговый путь всё равно
приходит — это нормально.

### Участие

1. Перед PR соберите проект и прогоните тесты (`ctest --test-dir build`).
2. Соблюдайте слои (`core/` ↔ `ui/`) и выносите новую логику в `core/`, чтобы её
   можно было тестировать без GUI.
3. Строки интерфейса могут быть на русском, но идентификаторы и комментарии
   держите в стиле того файла, который правите.

В баг-репорте полезны: ОС, версия Qt, `yt-dlp --version`, точная команда из лога
и что вы ожидали увидеть.

### Правовая оговорка

FLACORE — это плеер. Он не содержит контента и ничего не раздаёт. Интеграция с
yt-dlp — удобная обёртка над сторонним инструментом: вы сами отвечаете за то,
что скачиваете, за соблюдение условий использования сайтов и законодательства
об авторском праве своей страны. Не используйте приложение для распространения
чужих материалов.

### Лицензия

GPL-3.0 — см. [LICENSE](LICENSE).

### Контрибьюторы

- [@Kiruimaru](https://github.com/kiruimaru) — poopshit dev
- [@nex1door](https://github.com/nex1door) — absolute vibe-coder and larper
- [Claude](https://www.anthropic.com/claude) — mf that ate my whole balance & tokens
