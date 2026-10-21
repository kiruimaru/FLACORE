# Заглушка yt-dlp: тот же CLI-контракт, что использует YtDlp
add_executable(fake_ytdlp fake_ytdlp.cpp)

# Тест обёртки: сеть и YouTube не нужны
add_executable(ytdlp_test
    ytdlp_test.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ytdlp/YtDlp.cpp
)

target_include_directories(ytdlp_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ytdlp_test PRIVATE Qt6::Core)
target_compile_definitions(ytdlp_test PRIVATE FAKE_YTDLP_PATH="$<TARGET_FILE:fake_ytdlp>")
add_dependencies(ytdlp_test fake_ytdlp)

if(MSVC)
    target_compile_options(ytdlp_test PRIVATE /W3)
else()
    target_compile_options(ytdlp_test PRIVATE -Wall -Wextra)
endif()

add_test(NAME ytdlp_test COMMAND ytdlp_test)
