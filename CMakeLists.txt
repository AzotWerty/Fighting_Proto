cmake_minimum_required(VERSION 3.20)
project(NeonBrawl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(NeonBrawl src/main.cpp)
target_link_libraries(NeonBrawl PRIVATE raylib)
target_link_options(NeonBrawl PRIVATE -static -static-libgcc -static-libstdc++)

if (WIN32)
    target_link_libraries(NeonBrawl PRIVATE winmm)
endif()
