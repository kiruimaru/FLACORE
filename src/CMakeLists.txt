# Исходные файлы
set(SOURCES
    main.cpp
    # Позже добавите больше файлов
)

# Создать исполняемый файл
add_executable(flacore ${SOURCES})

# Линковка с Qt
target_link_libraries(flacore
    Qt6::Core
    Qt6::Multimedia
    Qt6::Widgets
    ${FFMPEG_LIBRARIES}
)

# Включить заголовочные файлы
target_include_directories(flacore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)