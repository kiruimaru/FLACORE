# Заглушка yt-dlp: тот же CLI-контракт, что использует YtDlp
add_executable(fake_ytdlp fake_ytdlp.cpp)

# --- тест обёртки yt-dlp: сеть и YouTube не нужны ---------------------------
add_executable(ytdlp_test ytdlp_test.cpp)
target_link_libraries(ytdlp_test PRIVATE flacore_lib Qt6::Core)
target_compile_definitions(ytdlp_test PRIVATE FAKE_YTDLP_PATH="$<TARGET_FILE:fake_ytdlp>")
add_dependencies(ytdlp_test fake_ytdlp)

# --- тест чтения тегов и палитры обложки ------------------------------------
add_executable(media_test media_test.cpp)
target_link_libraries(media_test PRIVATE flacore_lib Qt6::Core Qt6::Gui)

# --- тест чартов: разбор Deezer и Apple, чарт SoundCloud через заглушку ----
add_executable(charts_test charts_test.cpp)
target_link_libraries(charts_test PRIVATE flacore_lib Qt6::Core Qt6::Gui Qt6::Network)
target_compile_definitions(charts_test PRIVATE FAKE_YTDLP_PATH="$<TARGET_FILE:fake_ytdlp>")
add_dependencies(charts_test fake_ytdlp)

# --- живой прогон «Обзора»: настоящая сеть и yt-dlp (вручную, не в ctest) ----
add_executable(live_probe live_probe.cpp)
target_link_libraries(live_probe PRIVATE flacore_lib Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)
if(MSVC)
    target_compile_options(live_probe PRIVATE /W3)
else()
    target_compile_options(live_probe PRIVATE -Wall -Wextra)
endif()

# --- тест темы: разбор theme.json, токены, свой CSS, пример из репозитория ---
add_executable(theme_test theme_test.cpp)
target_link_libraries(theme_test PRIVATE flacore_lib Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(theme_test PRIVATE FLACORE_REPO_DIR="${CMAKE_SOURCE_DIR}")

# --- интерфейс: каркас, плейлист и все встроенные темы в offscreen -----------
add_executable(ui_test ui_test.cpp)
target_link_libraries(ui_test PRIVATE flacore_lib Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)

foreach(target ytdlp_test media_test charts_test theme_test ui_test)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()

add_test(NAME ytdlp_test COMMAND ytdlp_test)
add_test(NAME media_test COMMAND media_test)
add_test(NAME charts_test COMMAND charts_test)
add_test(NAME theme_test COMMAND theme_test -platform offscreen)
add_test(NAME ui_test COMMAND ui_test -platform offscreen)
