cmake_minimum_required(VERSION 3.21)
project(FLACORE VERSION 0.3.0 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматический MOC/RCC для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Найти Qt6 (Gui нужен явно: QDesktopServices/QColor)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)

# libav* в коде не используются: yt-dlp вызывает бинарник ffmpeg сам,
# а QMediaPlayer на Linux и так работает через ffmpeg-бэкенд.
# Опция оставлена на будущее (например, для чтения тегов через libavformat).
option(FLACORE_USE_FFMPEG_LIBS "Линковать libav* (код их пока не использует)" OFF)
if(FLACORE_USE_FFMPEG_LIBS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
        libavcodec
        libavformat
        libavutil
        libswresample
    )
endif()

# Добавить поддиректории
add_subdirectory(src)

# Тесты (сеть не нужны: yt-dlp подменяется заглушкой)
option(FLACORE_BUILD_TESTS "Собирать автотесты" ON)
if(FLACORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()