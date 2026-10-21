cmake_minimum_required(VERSION 3.20)
project(FLACORE VERSION 0.1.0 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматический MOC/UIC/RCC для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Найти Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Widgets)

# Найти FFmpeg через pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
    libswresample
)

# Включить директории заголовков FFmpeg
include_directories(${FFMPEG_INCLUDE_DIRS})

# Добавить поддиректории
add_subdirectory(src)

# Опционально: тесты
# enable_testing()
# add_subdirectory(tests)