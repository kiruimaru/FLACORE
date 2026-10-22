# Исходные файлы
set(SOURCES
    main.cpp
    core/player/Playlist.cpp
    core/ytdlp/YtDlp.cpp
    ui/MainWindow.cpp
    ui/YouTubePanel.cpp
    ui/YtDlpSettingsDialog.cpp
)

set(HEADERS
    core/player/Playlist.h
    core/ytdlp/YtDlp.h
    core/ytdlp/YtDlpTypes.h
    ui/MainWindow.h
    ui/YouTubePanel.h
    ui/YtDlpSettingsDialog.h
)

# Создать исполняемый файл (qt_add_executable выставляет WinMain/манифест на Windows)
qt_add_executable(flacore ${SOURCES} ${HEADERS})

# Линковка с Qt
target_link_libraries(flacore PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Widgets
)

# Включить заголовочные файлы
target_include_directories(flacore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(FLACORE_USE_FFMPEG_LIBS)
    target_link_libraries(flacore PRIVATE ${FFMPEG_LIBRARIES})
    target_include_directories(flacore PRIVATE ${FFMPEG_INCLUDE_DIRS})
endif()

if(MSVC)
    target_compile_options(flacore PRIVATE /W3 /permissive-)
else()
    target_compile_options(flacore PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    set_target_properties(flacore PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS flacore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/flacore.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/flacore.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()
