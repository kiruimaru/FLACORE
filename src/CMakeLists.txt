# Исходники ядра и интерфейса живут в статической библиотеке: так их собирают
# и приложение, и автотесты, без второй копии списка файлов.
set(SOURCES
    core/media/CoverPalette.cpp
    core/media/MediaTags.cpp
    core/online/Charts.cpp
    core/online/CoverLoader.cpp
    core/player/Playlist.cpp
    core/ytdlp/YtDlp.cpp
    ui/CoverFetchDialog.cpp
    ui/MainWindow.cpp
    ui/YouTubePanel.cpp
    ui/YtDlpSettingsDialog.cpp
    ui/YtDlpSettingsForm.cpp
    ui/pages/AppearancePage.cpp
    ui/pages/DiscoverPage.cpp
    ui/pages/LibraryPage.cpp
    ui/pages/ThemeTile.cpp
    ui/pages/YtDlpPage.cpp
    ui/theme/StyleSheet.cpp
    ui/theme/Theme.cpp
    ui/theme/ThemeManager.cpp
    ui/theme/ThemeUtil.cpp
    ui/widgets/AuroraBackdrop.cpp
    ui/widgets/GlassWidgets.cpp
    ui/widgets/Icons.cpp
    ui/widgets/PlayerBar.cpp
    ui/widgets/ShelfDelegate.cpp
    ui/widgets/TitleBar.cpp
    ui/widgets/TrackDelegate.cpp
)

set(HEADERS
    core/media/CoverPalette.h
    core/media/MediaTags.h
    core/online/Charts.h
    core/online/CoverLoader.h
    core/player/Playlist.h
    core/ytdlp/YtDlp.h
    core/ytdlp/YtDlpTypes.h
    ui/CoverFetchDialog.h
    ui/MainWindow.h
    ui/YouTubePanel.h
    ui/YtDlpSettingsDialog.h
    ui/YtDlpSettingsForm.h
    ui/pages/AppearancePage.h
    ui/pages/DiscoverPage.h
    ui/pages/LibraryPage.h
    ui/pages/ThemeTile.h
    ui/pages/YtDlpPage.h
    ui/theme/StyleSheet.h
    ui/theme/Theme.h
    ui/theme/ThemeManager.h
    ui/theme/ThemeUtil.h
    ui/widgets/AuroraBackdrop.h
    ui/widgets/GlassWidgets.h
    ui/widgets/Icons.h
    ui/widgets/PlayerBar.h
    ui/widgets/ShelfDelegate.h
    ui/widgets/TitleBar.h
    ui/widgets/TrackDelegate.h
)

add_library(flacore_lib STATIC
    ${SOURCES}
    ${HEADERS}
    resources.qrc
)

target_include_directories(flacore_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(flacore_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Multimedia
    Qt6::Widgets
)

target_compile_definitions(flacore_lib PUBLIC FLACORE_VERSION="${PROJECT_VERSION}")

if(FLACORE_USE_FFMPEG_LIBS)
    target_link_libraries(flacore_lib PUBLIC ${FFMPEG_LIBRARIES})
    target_include_directories(flacore_lib PUBLIC ${FFMPEG_INCLUDE_DIRS})
endif()

if(MSVC)
    target_compile_options(flacore_lib PRIVATE /W3 /permissive-)
else()
    target_compile_options(flacore_lib PRIVATE -Wall -Wextra)
endif()

# Создать исполняемый файл (qt_add_executable выставляет WinMain/манифест на Windows)
qt_add_executable(flacore main.cpp)

target_link_libraries(flacore PRIVATE flacore_lib)

if(MSVC)
    target_compile_options(flacore PRIVATE /W3 /permissive-)
else()
    target_compile_options(flacore PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    set_target_properties(flacore PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Установка: бинарник (рядом с ним обычно кладут Qt-плагины через windeployqt).
install(TARGETS flacore RUNTIME DESTINATION bin)