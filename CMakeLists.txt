cmake_minimum_required(VERSION 3.20)
project(GoalFriends LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Platform-free game logic (builds and tests on any OS) ----
add_library(gf_core STATIC
    src/core/json.cpp
    src/core/data.cpp
    src/core/battle.cpp
    src/core/game.cpp
    src/core/describe.cpp
    src/core/ability.cpp
    src/core/sprite.cpp)
target_include_directories(gf_core PUBLIC src/core)
if(MSVC)
    target_compile_options(gf_core PRIVATE /W3 /utf-8)
else()
    target_compile_options(gf_core PRIVATE -Wall -Wextra)
    target_compile_definitions(gf_core PUBLIC _GLIBCXX_ASSERTIONS)  # bounds-checked containers
endif()

# ---- Roster document generator (prints ROSTER.md from the game data)
add_executable(gf_roster_doc tools/dump_roster.cpp)
target_link_libraries(gf_roster_doc PRIVATE gf_core)

# ---- Tests ----
enable_testing()
add_executable(gf_tests tests/test_main.cpp)
target_link_libraries(gf_tests PRIVATE gf_core)
add_test(NAME gf_tests COMMAND gf_tests)
if(MINGW)
    target_link_options(gf_tests PRIVATE -static)
endif()

# ---- Windows desktop app ----
if(WIN32)
    add_executable(GoalFriends WIN32
        src/platform/main.cpp
        src/platform/gfx.cpp
        src/platform/ui.cpp)
    target_link_libraries(GoalFriends PRIVATE gf_core d2d1 dwrite shell32 user32 gdi32 ole32 uuid advapi32)
    target_compile_definitions(GoalFriends PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    if(MSVC)
        target_compile_options(GoalFriends PRIVATE /W3 /utf-8)
    else()
        target_compile_options(GoalFriends PRIVATE -Wall -Wextra)
        target_link_options(GoalFriends PRIVATE -static -mwindows)
    endif()
endif()
