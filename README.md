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

**Status:** 0.4.2 — early, actively breaking. The `Discover` section now shows live
charts (Deezer, Apple Music, SoundCloud). Read
[Known limitations](#known-limitations) before expecting it to behave like foobar2000.

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

On Windows, users who only want to run FLACORE should download the ready-made
`FLACORE-windows-x64.zip` from [GitHub Releases](https://github.com/kiruimaru/FLACORE/releases).
They do not need to install a compiler, Qt, CMake, or Python; see the Windows
steps in [Build](#build).

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
- Multi-file open dialog and **drag & drop** of files onto the track list
- Tags are read (FLAC/MP3/WAV): title, artist, album, duration and embedded cover
- Play / pause / stop / next / previous, drag-to-seek, volume
- **Auto-advance** to the next track, readable errors on broken files
- Double-click a playlist row to play it, `Ctrl+O`/`Space`/`Ctrl+←`/`Ctrl+→` shortcuts

### Appearance and UI

- **Glass layout**: custom window frame, sidebar sections, floating player bar,
  aurora backdrop that reacts to the accent colour
- **Themes are packages** — `theme.json` + optional `theme.qss` + `assets/`, five
  built in (`liquid-glass`, `midnight`, `carbon`, `sugar-cloud`, `neon`), your own
  dropped into the themes folder and picked from `Appearance → Themes`
- **Tune everything**: accent colour (or take it from the track's cover art),
  background image from a file or a URL, font family and scale, corner radius,
  glass density, blur, animations, performance mode
- **Your own CSS** over the theme, applied live, with tokens (`@accent`, `@radius`,
  `@font-sans`, …) and stable hooks such as `QWidget[dataUi="player"]`
- **Way out if a style breaks the UI**: `Ctrl+Alt+Shift+C` or `--no-custom-css`
- Search filter in the header, toast messages instead of a status bar
- Full reference: [docs/THEMES.md](docs/THEMES.md)

### Online (yt-dlp)

- **Search** on YouTube or SoundCloud from the `YouTube` tab (15 results)
- **Paste a link** and get the track without searching — URL detection is automatic
- **Listen** — downloads the best audio stream into a local cache and starts
  playback; a second click reuses the cached file
- **Save to library** — extracts audio to FLAC (default), MP3, Opus or M4A,
  embeds metadata, and drops the track into the playlist
- Download a playlist URL in one job and add all downloaded tracks to the library
- Live download progress (percent, speed, ETA), a cancel button, and readable
  errors instead of silent failure

### Discover — live charts

- `Discover` is the first section on the left: shelves with **live charts** from
  open endpoints that need no registration and no API key
- **Deezer chart** — worldwide top or by genre (17 genres: pop, rap, electronic,
  dance, rock, R&B, jazz, classical, metal, Russian music, …), 30 tracks each
- **Apple Music chart** — most-played or new music by country (US, UK, DE, FR, RU)
- **SoundCloud chart** — read through `yt-dlp` (`--flat-playlist`); if SoundCloud
  is unreachable (it answers 404 to Russian IPs), the shelf says so instead of
  pretending to be empty
- Tracks show artist, duration and source; cover art is fetched in the background
  and cached in `covercache`, so the page paints instantly next time
- Right-click a row: play a **30-second preview** immediately (Deezer and Apple
  serve one), **listen in full** or **save to library** through yt-dlp, open the
  track page, or copy `artist - title`
- **Listen / Save from a chart** reuse the YouTube tab: the row becomes a search
  query on the chosen source, the first playable match is downloaded into the
  cache and starts playing. If a match is DRM-protected or dead, the next
  candidates are tried (up to three, and the source switches YouTube ↔ SoundCloud)
- `FLACORE_OFFLINE=1` keeps the app off the network completely — the shelves stay
  empty and nothing is requested
- The engine lives in `core/online/`: `Charts` (URLs, parsers, catalogues) and
  `CoverLoader` (async covers with a disk cache), so it is testable without a UI

### Under the hood

- `YtDlp` is an async `QProcess` wrapper — no shell, no injection, no blocking UI
- One process at a time; the UI disables itself while a job is running
- No FFmpeg **libraries** required to build; the `yt-dlp` and `ffmpeg`
  executables are called at runtime

---

## Screenshots
<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/2ccf4975-9897-4b1b-a622-5130b08bc6d7" />
<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/a1dd10f1-5974-4049-832e-7438635da1b6" />
<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/f40ff7ab-c946-427c-8c7c-4e2160fc5cf4" />


---

## Requirements

| What | Why | Version |
|------|-----|---------|
| C++ compiler | build | C++17 (GCC 11+, Clang 14+, MSVC 2019+) |
| CMake | build | 3.21+ |
| Qt 6: `Core`, `Gui`, `Widgets`, `Multimedia`, `Network` | UI, playback and background image download | 6.5+ (tested on 6.12) |
| `yt-dlp` | online search / download | recent (tested 2026.08.19) |
| `ffmpeg` binary | **only** for saving/converting audio to FLAC/MP3/Opus/M4A | 4.x+ |
| Qt Multimedia backend | actual audio output | `qt6-multimedia-ffmpeg` on Linux |

`ffmpeg` is optional: listening works without it because the stream is cached
as-is. Saving to the library in FLAC/MP3/Opus/M4A needs it for audio
post-processing. Local playback does not need the separate `ffmpeg` executable.

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

### Windows — run the ready-made release (recommended for users)

No development tools are needed to run the release build:

| What you want to do | What must be installed |
|---|---|
| Play files already on the computer | Nothing extra |
| Search/download from YouTube (full current support) | Included in the release ZIP (`yt-dlp.exe` + Deno) |
| Search/download from SoundCloud | Included in the release ZIP (`yt-dlp.exe`) |
| Save/convert downloads to FLAC, MP3, Opus or M4A | Included in the release ZIP (`ffmpeg.exe` + `ffprobe.exe`) |
| Compile FLACORE from source | Git, Visual Studio C++ tools, Qt 6.8.3 MSVC 2022 x64, CMake |

The release ZIP includes portable `yt-dlp.exe`, Deno, FFmpeg and FFprobe in its
`tools` folder, so no Python, package manager, or separate install is needed
for online search, downloads, or audio conversion. FFmpeg is not needed for
local playback.

1. Open [GitHub Releases](https://github.com/kiruimaru/FLACORE/releases) and
   download `FLACORE-windows-x64.zip` from the release's **Assets** list. Do
   not download the automatically generated **Source code** archive.
2. In File Explorer, right-click the ZIP and choose **Extract All**. Extract
   the whole archive; do not try to run `flacore.exe` from inside the ZIP.
3. In the extracted files, open the folder that contains `flacore.exe` (it may
   be inside a `Release` subfolder) and run it. Keep the other files and
   subfolders there; they contain the Qt libraries and plugins.

The archive is for 64-bit Windows and includes the Qt runtime files and the
Microsoft compiler runtime deployed by the release workflow. Do not separately
install Qt, Visual Studio, CMake, MSYS2, or Python just to use this archive.
Local music playback works without the online tools below.

For YouTube and SoundCloud, just run FLACORE from the extracted release folder.
The ZIP already contains `yt-dlp.exe`, Deno, FFmpeg and FFprobe, and FLACORE
discovers them in the adjacent `tools` folder. Keep that folder beside
`flacore.exe`.

If you need to refresh those tools manually, install Python 3.10 or newer, open
PowerShell in the extracted application folder, and run:

```powershell
py -3 setup_windows.py --force
```

The script downloads the official portable tools and verifies their published
checksums. The `setup_windows.bat` file runs the same helper by double-clicking.

The ready-made release already includes FFmpeg. If you are using an older
archive or a source checkout, the helper can download it too (it is a large
download):

```powershell
py -3 setup_windows.py --ffmpeg
```

This places both `ffmpeg.exe` and `ffprobe.exe` in `tools/ffmpeg/bin`, where
FLACORE finds them automatically. If the application still reports that
`ffprobe` is missing, close FLACORE, run this command from the folder containing
`setup_windows.py`, and start the application again.
Alternatively, use `winget install --id Gyan.FFmpeg -e` and select its folder
in the `ffmpeg` field on the `yt-dlp` page if it is not detected automatically.

If Windows reports a missing `VCRUNTIME` or `MSVCP` DLL, install the latest
[Microsoft Visual C++ Redistributable for x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
and restart FLACORE. If SmartScreen warns that the publisher is unknown, first
verify that the ZIP came from the FLACORE GitHub Releases page; the release is
not code-signed.

### Windows — build from source with MSYS2 (verified)

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

### Windows — build from source with Visual Studio

To compile FLACORE yourself, install these components first:

1. Install [Git for Windows](https://git-scm.com/download/win).
2. Install [Visual Studio Community 2022](https://visualstudio.microsoft.com/vs/community/)
   or [Visual Studio Build Tools 2022](https://visualstudio.microsoft.com/downloads/).
   In Visual Studio Installer, select the **Desktop development with C++**
   workload and a Windows 10 or 11 SDK. Make sure the MSVC v143 x64/x86 build
   tools are selected.
3. Install [Qt 6.8.3](https://www.qt.io/download-qt-installer-oss) with the Qt
   online installer. Expand Qt 6.8.3 and select **MSVC 2022 64-bit** plus the
   **Qt Multimedia** module. The other required modules (`Core`, `Gui`,
   `Widgets`, and `Network`) are included in this kit. The compiler and Qt kit
   must both be MSVC 2022 64-bit; do not select MinGW for this build.
4. Install [CMake 3.21 or newer](https://cmake.org/download/) and enable its
   option to add CMake to `PATH`.

Open **Developer PowerShell for VS 2022** (or **x64 Native Tools Command
Prompt**). Clone the repository and enter its folder:

```powershell
git clone https://github.com/kiruimaru/FLACORE.git
cd FLACORE
```

If Qt was installed somewhere other than `C:\Qt`, change the `$Qt` path below:

```powershell
$Qt = "C:/Qt/6.8.3/msvc2022_64"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_PREFIX_PATH=$Qt" -DFLACORE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

To run the locally built executable outside the Qt development environment,
deploy its Qt runtime files from the same PowerShell window:

```powershell
& "$Qt\bin\windeployqt.exe" --release --compiler-runtime `
  ".\build\src\Release\flacore.exe"
.\build\src\Release\flacore.exe
```

This source-build setup is not needed when using the ready-made release ZIP
above.

### FreeBSD / other Unices

The CMake files are portable; install `qt6-base`, `qt6-multimedia`, `cmake` and
`ninja` from ports/packages. This configuration is not tested by the maintainers.

### Install `yt-dlp` and `ffmpeg`

```bash
# yt-dlp
pipx install yt-dlp                # or: pip install --user -U yt-dlp
sudo pacman -S yt-dlp              # Arch
winget install yt-dlp.yt-dlp       # Windows
winget install DenoLand.Deno       # JavaScript runtime for full YouTube support

# ffmpeg (only for saving/converting to FLAC/MP3/Opus/M4A)
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
   - To download a full playlist, paste its URL and enable «Плейлист целиком».
5. `Отмена` stops the current download; `Ctrl+Q` quits the app.

Pasting a direct link (`https://…`) into the search field skips the search and
fetches that single track.

### Browse live charts

1. Open the `Discover` section (it is the first one, shown on start).
2. Pick a genre for **Deezer** or a country and feed for **Apple Music** — the
   shelf reloads as soon as you change the dropdown, or press «Показать».
3. `Обновить всё` reloads every shelf, including the SoundCloud chart.
4. Select a row and press `Enter`/double-click to listen in full, or right-click
   for the menu: `▶ Превью 30 секунд`, `Слушать полностью`, `Сохранить в
   библиотеку`, `Открыть страницу трека`, `Скопировать «исполнитель — название»`.
5. Full playback happens on the `YouTube` tab, where progress is visible; the
   player picks the file up as soon as it is downloaded.

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
| `appearance/theme` | active theme id | `liquid-glass` |
| `appearance/accent` | accent override (`#rrggbb`), empty = from the theme | empty |
| `appearance/customCss`, `appearance/customCssEnabled` | your CSS and its toggle | empty, on |
| `appearance/backgroundImage`, `appearance/backgroundBlur` | wallpaper and its blur | empty, theme value |
| `appearance/{radius,glassAlpha,blur,font,fontScale,mode,animations}` | appearance sliders | theme values |

Where it lands:

- Linux: `~/.config/flacore.conf`, cache in `~/.cache/flacore/ytdlp`
- Windows: registry `HKEY_CURRENT_USER\Software\flacore`, cache in
  `%LOCALAPPDATA%\flacore\cache\ytdlp`
- Your themes: `%APPDATA%\FLACORE\FLACORE\themes` on Windows,
  `~/.local/share/FLACORE/FLACORE/themes` on Linux (see [docs/THEMES.md](docs/THEMES.md))

Two environment variables change behaviour without touching the settings:

- `FLACORE_OFFLINE=1` — never touch the network (charts stay empty). Used by the
  tests, handy on a metered connection.
- `FLACORE_LIVE_YTDLP=<path>` — used by the manual live probe to point at `yt-dlp`.

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

What is covered (five test binaries, 304 assertions):

- `ytdlp_test` — the CLI contract against a compiled stub: availability probe,
  search JSON (playlist, direct URL, `uploader` → `channel`, live streams),
  `ytsearch` vs `scsearch`, download progress and final path, failures, cancel
- `media_test` — tag reading on fixtures built byte by byte (FLAC STREAMINFO +
  VORBIS_COMMENT + PICTURE, MP3 ID3v2 + MPEG frame, WAV), playlist metadata,
  cover palette extraction
- `theme_test` — `theme.json` parsing and validation, token derivation for dark
  and light themes, `@token` substitution, custom CSS sanitising, built stylesheet
  (no unresolved tokens), plus WCAG contrast checks for every built-in theme
- `charts_test` — the Discover engine on fixtures: Deezer and Apple chart JSON
  (artist-picture fallback, `durationInMillis`, `100x100bb` → `600x600bb`), error
  payloads and broken JSON, source URLs and limits, genre and country catalogues,
  `artist - title` queries, `yt-dlp` records → chart rows, and the SoundCloud
  chart read through the `yt-dlp` stub
- `ui_test` (offscreen) — window shell, playlist wired to the view, add/remove/
  clear, index shifts, all built-in themes applied on a live window, custom CSS,
  the `Ctrl+Alt+Shift+C` escape hatch, the Discover shelves fed with synthetic
  chart rows (roles, durations, `artist - title` routing, 30-second preview URL);
  it also saves screenshots

Turn tests off with `-DFLACORE_BUILD_TESTS=OFF`.

### Live probe (network, manual)

`tests/live_probe.cpp` is built but **not** registered with `ctest`: it needs the
real network and the real `yt-dlp`. It boots the whole window and checks what
fixtures cannot: Deezer and Apple charts actually arriving, a preview stream
actually playing, a chart row actually becoming a cached file in the playlist,
and the SoundCloud chart responding.

```bash
cd build/tests
FLACORE_LIVE_YTDLP=/path/to/yt-dlp ./live_probe.exe
```

---

## Project structure

```
FLACORE/
├── CMakeLists.txt                # Qt6 lookup, optional FFmpeg libs, tests
├── src/
│   ├── main.cpp                  # QApplication, --no-custom-css, ThemeManager
│   ├── CMakeLists.txt            # flacore_lib + qt_add_executable, -Wall -Wextra
│   ├── resources.qrc             # built-in themes (theme.json + theme.qss)
│   ├── core/
│   │   ├── media/                # MediaTags (FLAC/MP3/WAV tags, cover), CoverPalette
│   │   ├── online/               # Charts (Deezer/Apple charts, parsers), CoverLoader
│   │   ├── player/Playlist.*      # Track, list, current index, signals
│   │   └── ytdlp/
│   │       ├── YtDlpTypes.h      # YtDlpResult, formatDuration()
│   │       └── YtDlp.*           # QProcess wrapper: search/download/cancel
│   ├── themes/<id>/              # built-in themes: theme.json + theme.qss
│   └── ui/
│       ├── MainWindow.*          # shell: title bar, sidebar, pages, player bar
│       ├── YouTubePanel.*        # search UI, progress, play/save buttons
│       ├── YtDlpSettingsForm.*   # shared settings form
│       ├── YtDlpSettingsDialog.* # the same form as a dialog
│       ├── theme/                # Theme, StyleSheet, ThemeManager, ThemeUtil
│       ├── pages/                # Discover, Library, Appearance, ThemeTile, YtDlp
│       └── widgets/              # AuroraBackdrop, GlassPanel, CoverArt, SeekBar,
│                                 # TitleBar, PlayerBar, TrackDelegate, ShelfDelegate,
│                                 # Icons
├── themes/example/               # user theme to copy (see docs/THEMES.md)
├── docs/THEMES.md                # theme format, tokens, hooks, own CSS
├── tests/
│   ├── fake_ytdlp.cpp            # stub with the same CLI contract
│   ├── ytdlp_test.cpp            # yt-dlp wrapper, no network
│   ├── media_test.cpp            # tags and cover palette on synthetic fixtures
│   ├── theme_test.cpp            # theme JSON, tokens, custom CSS
│   ├── charts_test.cpp           # charts, parsers, yt-dlp records (fixtures)
│   ├── ui_test.cpp               # offscreen UI + screenshots
│   ├── live_probe.cpp            # manual run with the real network and yt-dlp
│   └── CMakeLists.txt
├── LICENSE                       # GPL-3.0
└── README.md
```

Layering rule: `core/` knows nothing about widgets, `ui/` never spawns processes
directly. `core/` + `ui/` build into the static library `flacore_lib`, so the
tests link the very same code the app runs. `Playlist` owns the state and emits
signals; pages only render it and report user intents.

---

## Known limitations

An honest list — these are real, not hypothetical:

- **Playlist is in memory.** No M3U import/export, no persistence between runs.
- **No shuffle / repeat / queue.**
- **No media keys, no MPRIS, no tray icon.**
- **No packaging.** No AppImage, Flatpak, MSI or `.desktop` file yet.
- **The "glass" is not a real backdrop filter.** Qt Widgets cannot blur what is
  behind a widget, so panels are translucent and the backdrop image itself is
  blurred. Looks close, costs nothing.
- **Tags are read for FLAC/MP3/WAV only.** M4A/AAC/OGG show the file name.
- **YouTube may be unreachable** from your network (it is from Russia without a
  proxy). Use the SoundCloud source, or add `--proxy` to `yt-dlp`'s own config.
- **Charts come from public, undocumented endpoints** (Deezer, the Apple Marketing
  Tools RSS feed). They need no API key — and that is also their weakness: the
  formats can change without notice, and the endpoints can be blocked.
- **SoundCloud charts are read through `yt-dlp`.** SoundCloud answers 404 to
  `soundcloud.com/charts` from some regions (Russia included), so that shelf shows
  an error there; the Deezer and Apple shelves keep working.
- **The feed is not personalised.** No likes, no listening history, no
  recommendations — just charts.

---

## Roadmap

- [x] auto-advance and a proper transport state (`refreshTransport()`, `EndOfMedia`)
- [ ] `PlaylistModel : QAbstractListModel` + `QListView` (drag & drop, roles)
- [x] tag and cover reading (FLAC/MP3/WAV), accent from the cover palette
- [x] theme system: packages, tokens, own CSS, escape hatch — see `docs/THEMES.md`
- [x] Discover section: live charts (Deezer / Apple Music / SoundCloud via yt-dlp)
- [ ] personalised feed: likes, history, recommendations
- [ ] 30-second previews without leaving the Discover page (they currently play in
      the main player bar)
- [ ] M3U save/load, shuffle, repeat
- [x] Download playlist URLs and add all downloaded tracks to the library
- [x] Theme presets, custom color editors, JSON configs and background images
- [x] Feedback link to GitHub Issues
- [ ] download queue instead of one job at a time
- [ ] MPRIS2 + media keys, tray icon
- [ ] GitHub Actions: build on Linux + the four test binaries
- [ ] packaging: AppImage / Flatpak / Windows installer

---

## Troubleshooting

**«yt-dlp не найден»** — the binary is not in `PATH`. Set the full path in the
`yt-dlp` section on the left (or `Меню → Настройки yt-dlp…`). On Windows
`pip install` puts it into
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

**Charts stay empty / «Deezer недоступен»** — the app could not reach the chart
sources. Check the internet, a proxy/VPN, or run with `FLACORE_OFFLINE=1` if you
want the app to stay off the network on purpose. The SoundCloud shelf failing with
a 404 while Deezer works means SoundCloud is blocked for your region, not a bug.

**A chart row plays the wrong track or fails** — the row is turned into an
`artist - title` search on the source chosen on the YouTube tab. If the first hit
is DRM-protected, the app automatically tries the next candidates (up to three)
and then the other source. If everything fails, use `Открыть страницу трека` and
paste the link into the search field instead.

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

**Статус:** 0.4.2 — ранняя версия, всё ещё ломается. В разделе «Обзор» появились
живые чарты (Deezer, Apple Music, SoundCloud). Перед тем как ждать поведения
foobar2000, прочитайте [Ограничения](#ограничения).

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
- Открытие сразу нескольких файлов и **перетаскивание** файлов в список
- Теги читаются (FLAC/MP3/WAV): название, исполнитель, альбом, длительность, обложка
- Play / пауза / стоп / следующий / предыдущий, перемотка перетаскиванием, громкость
- **Автопереход** на следующий трек, понятные ошибки на битых файлах
- Двойной клик по строке плейлиста запускает трек; горячие клавиши `Ctrl+O`, `Space`, `Ctrl+←/→`

**Оформление и интерфейс**

- **Стеклянная раскладка**: своя шапка окна, разделы слева, плавающая панель
  плеера, живой фон, который реагирует на акцент
- **Темы — это пакеты**: `theme.json` + необязательный `theme.qss` + `assets/`,
  пять встроенных (`liquid-glass`, `midnight`, `carbon`, `sugar-cloud`, `neon`),
  свои кладутся в папку тем и выбираются в «Оформление → Тема»
- **Настраивается всё**: акцент (в том числе из обложки трека), фоновое
  изображение из файла или по ссылке, шрифт и его масштаб, скругление,
  плотность стекла, размытие, анимации, режим производительности
- **Свой CSS** поверх темы, применяется сразу, с токенами (`@accent`, `@radius`,
  `@font-sans`, …) и стабильными хуками вида `QWidget[dataUi="player"]`
- **Если стиль сломал интерфейс**: `Ctrl+Alt+Shift+C` или запуск с `--no-custom-css`
- Поиск-фильтр в шапке, всплывающие сообщения вместо строки состояния
- Полное описание: [docs/THEMES.md](docs/THEMES.md)

**Онлайн (yt-dlp)**

- **Поиск** по YouTube или SoundCloud на вкладке `YouTube` (15 результатов)
- **Вставка ссылки** — трек без поиска, URL распознаётся автоматически
- **«Слушать»** — лучший аудиопоток скачивается в локальный кэш и сразу играет;
  повторное нажатие переиспользует уже скачанный файл
- **«В библиотеку»** — извлечение аудио в FLAC (по умолчанию), MP3, Opus или M4A
  с вшиванием метаданных; трек сразу попадает в плейлист
- Живой прогресс (проценты, скорость, ETA), кнопка отмены и понятные ошибки
  вместо молчаливого отказа

**Обзор — живая подборка**

- `Обзор` — первый раздел слева: полки с **живыми чартами** из открытых
  источников, без регистрации и без API-ключей
- **Чарт Deezer** — топ по всему миру или по жанру (17 жанров: поп, рэп,
  электронная, танцевальная, рок, R&B, джаз, классика, металл, русская музыка и
  другие), по 30 треков
- **Чарт Apple Music** — самые прослушиваемые или новинки по стране
  (США, Великобритания, Германия, Франция, Россия)
- **Чарт SoundCloud** — читается через `yt-dlp` (`--flat-playlist`); если
  SoundCloud недоступен (из России он отвечает 404), полка честно об этом пишет,
  а не притворяется пустой
- В строках видны исполнитель, длительность и источник; обложки докачиваются в
  фоне и кладутся в кэш `covercache`, поэтому страница открывается мгновенно
- Правый клик по строке: **превью на 30 секунд** сразу (его отдают Deezer и
  Apple), **слушать полностью** или **сохранить в библиотеку** через yt-dlp,
  открыть страницу трека, скопировать «исполнитель — название»
- **«Слушать» и «В библиотеку» из чарта** работают через вкладку `YouTube`:
  строка превращается в поисковый запрос на выбранном источнике, первый живой
  результат скачивается в кэш и сразу играет. Если он под DRM или удалён,
  приложение само пробует следующие (до трёх) и переключает источник
  YouTube ↔ SoundCloud
- `FLACORE_OFFLINE=1` полностью отключает сеть — полки остаются пустыми, запросов
  нет
- Движок лежит в `core/online/`: `Charts` (ссылки, парсеры, справочники) и
  `CoverLoader` (асинхронные обложки с кэшем на диске) — тестируется без GUI

**Под капотом**

- `YtDlp` — асинхронная обёртка над `QProcess`: без shell, без инъекций,
  интерфейс не блокируется
- Один процесс за раз; на время работы элементы управления отключаются
- Библиотеки FFmpeg для сборки **не нужны** — вызываются исполняемые файлы
  `yt-dlp` и `ffmpeg`

### Скриншоты

<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/2ccf4975-9897-4b1b-a622-5130b08bc6d7" />
<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/a1dd10f1-5974-4049-832e-7438635da1b6" />
<img width="1300" height="1000" alt="image" src="https://github.com/user-attachments/assets/f40ff7ab-c946-427c-8c7c-4e2160fc5cf4" />

### Требования

| Что | Зачем | Версия |
|-----|-------|--------|
| Компилятор C++ | сборка | C++17 (GCC 11+, Clang 14+, MSVC 2019+) |
| CMake | сборка | 3.21+ |
| Qt 6: `Core`, `Gui`, `Widgets`, `Multimedia`, `Network` | интерфейс, воспроизведение, загрузка фона по ссылке | 6.5+ (проверено на 6.12) |
| `yt-dlp` | поиск и скачивание | свежий (проверено 2026.08.19) |
| бинарник `ffmpeg` | **только** для сохранения/конвертации в FLAC/MP3/Opus/M4A | 4.x+ |
| бэкенд Qt Multimedia | собственно звук | `qt6-multimedia-ffmpeg` на Linux |

`ffmpeg` необязателен: прослушивание работает без него, потому что поток
кэшируется как есть. Сохранение в библиотеку в форматах FLAC/MP3/Opus/M4A
требует `ffmpeg` для обработки аудио. Для локального воспроизведения отдельный
бинарник `ffmpeg` не нужен.

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

**Windows — запуск готового релиза (рекомендуется обычным пользователям)**

<img width="411" height="393" alt="image" src="https://github.com/user-attachments/assets/0cb27cf7-98c9-4309-bd1d-25a920d7ba35" />


Для запуска не нужны инструменты разработки:

| Что нужно делать | Что установить |
|---|---|
| Слушать файлы, которые уже есть на компьютере | Ничего дополнительно |
| Искать/скачивать с YouTube (полная актуальная поддержка) | Уже в ZIP релиза (`yt-dlp.exe` + Deno) |
| Искать/скачивать с SoundCloud | Уже в ZIP релиза (`yt-dlp.exe`) |
| Сохранять/конвертировать скачанное в FLAC, MP3, Opus или M4A | Уже в ZIP релиза (`ffmpeg.exe` + `ffprobe.exe`) |
| Собирать FLACORE из исходников | Git, инструменты C++ Visual Studio, Qt 6.8.3 MSVC 2022 x64, CMake |

В ZIP релиза уже есть переносимые `yt-dlp.exe`, Deno, FFmpeg и FFprobe в папке
`tools`, поэтому для поиска, скачивания и конвертации музыки не нужно отдельно
устанавливать Python, менеджер пакетов или эти инструменты. Для локального
воспроизведения FFmpeg не требуется.

1. Откройте [GitHub Releases](https://github.com/kiruimaru/FLACORE/releases) и
   скачайте `FLACORE-windows-x64.zip` из списка **Assets** у нужного релиза.
   Не скачивайте автоматически созданный архив **Source code**.
2. В Проводнике нажмите ZIP правой кнопкой и выберите **Извлечь все**. Извлеките
   весь архив; не запускайте `flacore.exe` прямо внутри ZIP.
3. Найдите в распакованных файлах папку с `flacore.exe` (он может быть во
   вложенной папке `Release`) и запустите его. Не удаляйте и не переносите
   отдельно от него остальные файлы и папки: там находятся Qt DLL и плагины.

Архив собран для 64-разрядной Windows и содержит файлы Qt и распространяемую
среду Microsoft C++, добавленные при сборке релиза. Для запуска архива не нужно
отдельно ставить Qt, Visual Studio, CMake, MSYS2 или Python. Локальную музыку
можно слушать без дополнительных онлайн-инструментов.

Для YouTube и SoundCloud просто запустите FLACORE из распакованной папки
релиза. В ZIP уже находятся `yt-dlp.exe`, Deno, FFmpeg и FFprobe; FLACORE
находит их в соседней папке `tools`. Не переносите `tools` отдельно от
`flacore.exe`.

Если нужно вручную обновить инструменты, установите Python 3.10 или новее,
откройте PowerShell в папке приложения и выполните:

```powershell
py -3 setup_windows.py --force
```

Скрипт скачает официальные переносимые версии инструментов и проверит
опубликованные контрольные суммы. То же самое запускает двойной щелчок по
`setup_windows.bat`.

Готовый архив релиза уже содержит FFmpeg. Если используется старый архив или
сборка из исходников, скачайте его вручную (это большой файл):

```powershell
py -3 setup_windows.py --ffmpeg
```

Это скачает `ffmpeg.exe` и `ffprobe.exe` в `tools/ffmpeg/bin`, откуда FLACORE
определит их автоматически. Также можно установить FFmpeg через `winget
install --id Gyan.FFmpeg -e`; если программа не найдёт его сама, укажите папку
с `ffmpeg.exe` в разделе `yt-dlp`.

Если Windows сообщает об отсутствующей DLL `VCRUNTIME` или `MSVCP`, установите
последнюю [Microsoft Visual C++ Redistributable для x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
и перезапустите FLACORE. Если SmartScreen сообщает о неизвестном издателе,
сначала убедитесь, что архив скачан со страницы GitHub Releases проекта: у
релиза нет цифровой подписи.

**Windows — сборка из исходников через MSYS2 (проверено)**

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

**Windows — сборка из исходников через Visual Studio**

Если хотите самостоятельно скомпилировать FLACORE, сначала установите:

1. Установите [Git for Windows](https://git-scm.com/download/win).
2. Установите [Visual Studio Community 2022](https://visualstudio.microsoft.com/vs/community/)
   или [Visual Studio Build Tools 2022](https://visualstudio.microsoft.com/downloads/).
   В Visual Studio Installer выберите компонент
   **Разработка классических приложений на C++** и Windows 10/11 SDK. Проверьте,
   что также выбран компилятор MSVC v143 для x64/x86.
3. Установите [Qt 6.8.3](https://www.qt.io/download-qt-installer-oss) через
   онлайн-установщик Qt. В списке Qt 6.8.3 отметьте комплект **MSVC 2022
   64-bit** и модуль **Qt Multimedia**. Остальные необходимые модули (`Core`,
   `Gui`, `Widgets`, `Network`) входят в этот комплект. Для этой сборки и
   компилятор, и Qt должны быть MSVC 2022 64-bit; вариант MinGW не подходит.
4. Установите [CMake версии 3.21 или новее](https://cmake.org/download/) и
   включите добавление CMake в `PATH`.

Откройте **Developer PowerShell for VS 2022** (или **x64 Native Tools Command
Prompt**). Склонируйте проект и перейдите в его папку:

```powershell
git clone https://github.com/kiruimaru/FLACORE.git
cd FLACORE
```

Если Qt установлен не в `C:\Qt`, измените путь в переменной `$Qt`:

```powershell
$Qt = "C:/Qt/6.8.3/msvc2022_64"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_PREFIX_PATH=$Qt" -DFLACORE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Чтобы запускать собранную программу вне среды Qt, разверните необходимые Qt
DLL из того же PowerShell:

```powershell
& "$Qt\bin\windeployqt.exe" --release --compiler-runtime `
  ".\build\src\Release\flacore.exe"
.\build\src\Release\flacore.exe
```

Эти шаги сборки не нужны, если вы скачали готовый ZIP релиза.

**FreeBSD и другие Unix**

CMake-файлы портируемы: поставьте `qt6-base`, `qt6-multimedia`, `cmake` и `ninja`
из портов/пакетов. Мейнтейнеры эту конфигурацию не тестировали.

**Установка `yt-dlp` и `ffmpeg`**

```bash
# yt-dlp
pipx install yt-dlp                # или: pip install --user -U yt-dlp
sudo pacman -S yt-dlp              # Arch
winget install yt-dlp.yt-dlp       # Windows
winget install DenoLand.Deno       # JavaScript runtime для полной поддержки YouTube

# ffmpeg (только для сохранения/конвертации в FLAC/MP3/Opus/M4A)
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
   - Чтобы скачать плейлист целиком, вставьте его URL и включите
     «Плейлист целиком».
5. «Отмена» прерывает текущее скачивание, `Ctrl+Q` закрывает приложение.

Если вставить в поле поиска прямую ссылку (`https://…`), поиск пропускается и
загружается именно этот трек.

**Живая подборка**

1. Откройте раздел `Обзор` (он первый и показывается при запуске).
2. Выберите жанр для **Deezer** или страну и ленту для **Apple Music** — полка
   обновляется сразу при смене списка, либо нажмите «Показать».
3. «Обновить всё» перезагружает все полки, включая чарт SoundCloud.
4. Выберите строку: `Enter`/двойной клик — слушать полностью; правый клик — меню
   «▶ Превью 30 секунд», «Слушать полностью», «Сохранить в библиотеку»,
   «Открыть страницу трека», «Скопировать „исполнитель — название“».
5. Полное воспроизведение идёт через вкладку `YouTube`, там виден прогресс;
   плеер подхватывает файл сразу после скачивания.

**Настройки**

«Инструменты → Настройки yt-dlp…» — путь к `yt-dlp`, папка с `ffmpeg`, папка
библиотеки и формат сохранения (FLAC / MP3 320 / Opus / M4A). Диалог сразу
проверяет `yt-dlp --version` и показывает результат. «Инструменты → Открыть
папку библиотеки» открывает её в файловом менеджере.

«Вид → Тема» предлагает готовые темы, редакторы цветов «Хардкор» и
«Интерактивный», импорт/экспорт JSON-конфига и выбор фото фона. Кнопка
`Feedback` открывает GitHub Issues.

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

Две переменные окружения меняют поведение без правки настроек:

- `FLACORE_OFFLINE=1` — не ходить в сеть вообще (полки чартов остаются пустыми).
  Используется тестами, пригодится на лимитированном трафике.
- `FLACORE_LIVE_YTDLP=<путь>` — путь к `yt-dlp` для ручного живого прогона.

Удалите эти данные, чтобы сбросить приложение к состоянию «как после установки».

### Тесты

Сеть, настоящий `yt-dlp` и звуковое устройство не нужны — контракт CLI
проверяется на скомпилированной заглушке (`tests/fake_ytdlp.cpp`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Что покрыто (пять тестовых бинарников, 304 проверок):

- `ytdlp_test` — контракт CLI на скомпилированной заглушке: проверка
  доступности, разбор JSON (плейлист, прямая ссылка, `uploader` → `channel`,
  live-стримы), `ytsearch` против `scsearch`, прогресс и итоговый путь,
  ошибки, отмена
- `media_test` — чтение тегов на фикстурах, собранных байт за байтом (FLAC
  STREAMINFO + VORBIS_COMMENT + PICTURE, MP3 ID3v2 + кадр MPEG, WAV),
  метаданные в плейлисте, палитра из обложки
- `theme_test` — разбор и проверка `theme.json`, вывод токенов для тёмной и
  светлой темы, подстановка `@токенов`, аварийный разбор своего CSS, сборка
  итогового стиля (не остаётся неподставленных токенов), плюс проверки
  контраста по WCAG для каждой встроенной темы
- `charts_test` — движок «Обзора» на фикстурах: ответы чартов Deezer и Apple
  (включая запасное фото исполнителя, `durationInMillis`, `100x100bb` →
  `600x600bb`), ответы с ошибкой и битый JSON, ссылки и ограничения источников,
  справочники жанров и стран, запросы «исполнитель — название», записи yt-dlp →
  строки чарта и чтение чарта SoundCloud через заглушку yt-dlp
- `ui_test` (offscreen) — каркас окна, связь плейлиста с представлением,
  добавление/удаление/очистка, сдвиг индексов, применение всех встроенных тем
  на живом окне, свой CSS, аварийное `Ctrl+Alt+Shift+C`, полки «Обзора» на
  подставленных строках чарта (роли, длительности, маршрутизация «исполнитель —
  название», ссылка превью); тест заодно сохраняет скриншоты

Отключить тесты: `-DFLACORE_BUILD_TESTS=OFF`.

### Живой прогон (сеть, вручную)

`tests/live_probe.cpp` собирается, но **не** регистрируется в `ctest`: ему нужны
настоящая сеть и настоящий `yt-dlp`. Он поднимает всё окно и проверяет то, что
фикстурами не проверить: приходят ли чарты Deezer и Apple, играет ли поток
превью, превращается ли строка чарта в скачанный файл в плейлисте и отвечает ли
чарт SoundCloud.

```bash
cd build/tests
FLACORE_LIVE_YTDLP=/path/to/yt-dlp ./live_probe.exe
```

### Структура проекта

```
FLACORE/
├── CMakeLists.txt                # поиск Qt6, опциональные libav*, тесты
├── src/
│   ├── main.cpp                  # QApplication, --no-custom-css, ThemeManager
│   ├── CMakeLists.txt            # flacore_lib + qt_add_executable, -Wall -Wextra
│   ├── resources.qrc             # встроенные темы (theme.json + theme.qss)
│   ├── core/
│   │   ├── media/                # MediaTags (теги FLAC/MP3/WAV, обложка), CoverPalette
│   │   ├── online/               # Charts (чарты Deezer/Apple, парсеры), CoverLoader
│   │   ├── player/Playlist.*      # Track, список, текущий индекс, сигналы
│   │   └── ytdlp/
│   │       ├── YtDlpTypes.h      # YtDlpResult, formatDuration()
│   │       └── YtDlp.*           # QProcess: search/download/cancel
│   ├── themes/<id>/              # встроенные темы: theme.json + theme.qss
│   └── ui/
│       ├── MainWindow.*          # каркас: шапка, сайдбар, страницы, панель плеера
│       ├── YouTubePanel.*        # поиск, прогресс, кнопки «Слушать»/«В библиотеку»
│       ├── YtDlpSettingsForm.*   # форма настроек yt-dlp (общая)
│       ├── YtDlpSettingsDialog.* # та же форма в виде диалога
│       ├── theme/                # Theme, StyleSheet, ThemeManager, ThemeUtil
│       ├── pages/                # страницы: обзор, библиотека, оформление, темы, yt-dlp
│       └── widgets/              # AuroraBackdrop, GlassPanel, CoverArt, SeekBar,
│                                 # TitleBar, PlayerBar, TrackDelegate, ShelfDelegate,
│                                 # Icons
├── themes/example/               # своя тема для копирования (см. docs/THEMES.md)
├── docs/THEMES.md                # формат тем, токены, хуки, свой CSS
├── tests/
│   ├── fake_ytdlp.cpp            # заглушка с тем же CLI-контрактом
│   ├── ytdlp_test.cpp            # обёртка yt-dlp, без сети
│   ├── media_test.cpp            # теги и палитра обложки на синтетических фикстурах
│   ├── theme_test.cpp            # theme.json, токены, свой CSS
│   ├── charts_test.cpp           # чарты, парсеры, записи yt-dlp (фикстуры)
│   ├── ui_test.cpp               # интерфейс в offscreen + скриншоты
│   ├── live_probe.cpp            # ручной прогон с настоящей сетью и yt-dlp
│   └── CMakeLists.txt
├── LICENSE                       # GPL-3.0
└── README.md
```

Правило слоёв: `core/` ничего не знает о виджетах, `ui/` никогда не запускает
процессы напрямую. `core/` и `ui/` собираются в статическую библиотеку
`flacore_lib`, поэтому тесты линкуют ровно тот же код, что и приложение.
Состоянием владеет `Playlist` и сообщает о нём сигналами; страницы только
показывают его и передают намерения пользователя.

### Ограничения

Честный список — это реальные вещи, а не гипотетические:

- **Плейлист живёт в памяти.** Нет импорта/экспорта M3U, нет сохранения между
  запусками.
- **Нет shuffle / repeat / очереди.**
- **Нет медиаклавиш, MPRIS и значка в трее.**
- **Нет упаковки.** Ни AppImage, ни Flatpak, ни MSI, ни `.desktop`.
- **«Стекло» — не настоящий backdrop-фильтр.** Qt Widgets не умеет размывать то,
  что находится за виджетом, поэтому панели полупрозрачные, а размывается сама
  картинка фона. Выглядит близко, стоит дёшево.
- **Теги читаются только для FLAC/MP3/WAV.** M4A/AAC/OGG показывают имя файла.
- **YouTube может быть недоступен** из вашей сети (например, из России без
  прокси). Используйте источник SoundCloud или добавьте `--proxy` в конфиг
  самого `yt-dlp`.
- **Чарты берутся из публичных недокументированных источников** (Deezer, RSS
  Apple Marketing Tools). Ключ не нужен — в этом и слабость: форматы могут
  поменяться без предупреждения, а сами адреса — блокироваться.
- **Чарт SoundCloud читается через `yt-dlp`.** Из ряда регионов (в том числе из
  России) `soundcloud.com/charts` отвечает 404, поэтому эта полка показывает
  ошибку; полки Deezer и Apple при этом работают.
- **Подборка не персонализирована.** Ни лайков, ни истории прослушиваний, ни
  рекомендаций — только чарты.

### Планы

- [x] автопереход и нормальное состояние транспорта (`refreshTransport()`, `EndOfMedia`)
- [ ] `PlaylistModel : QAbstractListModel` + `QListView` (drag & drop, роли)
- [x] чтение тегов и обложек (FLAC/MP3/WAV), акцент из палитры обложки
- [x] система тем: пакеты, токены, свой CSS, аварийный выход — см. `docs/THEMES.md`
- [x] раздел «Обзор»: живые чарты (Deezer / Apple Music / SoundCloud через yt-dlp)
- [ ] персональная подборка: лайки, история, рекомендации
- [ ] превью на 30 секунд прямо на странице «Обзор» (сейчас играет в общей панели)
- [ ] сохранение/загрузка M3U, shuffle, repeat
- [x] загрузка плейлиста целиком с добавлением треков в библиотеку
- [x] готовые темы, редакторы цветов, JSON-конфиги и фото фона
- [x] кнопка Feedback со ссылкой на GitHub Issues
- [ ] очередь скачивания вместо одной задачи
- [ ] MPRIS2 + медиаклавиши, значок в трее
- [ ] GitHub Actions: сборка на Linux + четыре тестовых бинарника
- [ ] упаковка: AppImage / Flatpak / установщик для Windows

### Решение проблем

**«yt-dlp не найден»** — бинарника нет в `PATH`. Укажите полный путь в разделе
«yt-dlp» слева (или «Меню → Настройки yt-dlp…»). На Windows `pip install` кладёт
его в `%LOCALAPPDATA%\Programs\Python\Python3xx\Scripts`, а этой папки часто нет
в `PATH`.

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

**Полки чартов пустые / «Deezer недоступен»** — приложение не смогло связаться с
источниками чартов. Проверьте интернет, прокси/VPN, либо запускайте с
`FLACORE_OFFLINE=1`, если сеть не нужна осознанно. Если полка SoundCloud даёт 404,
а Deezer работает — это блокировка SoundCloud для вашего региона, а не ошибка.

**Строка чарта играет не тот трек или не играет** — строка превращается в поиск
«исполнитель — название» на источнике, выбранном на вкладке YouTube. Если первый
найденный трек под DRM, приложение само пробует следующие (до трёх), а затем
другой источник. Если не вышло совсем — «Открыть страницу трека» и вставьте
ссылку в поле поиска.

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
