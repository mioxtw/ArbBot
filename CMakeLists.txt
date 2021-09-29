cmake_minimum_required(VERSION 3.10)
project(ftxclient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wfatal-errors -Wpedantic -Wextra -Wmissing-include-dirs -Wconversion -Wsign-conversion -pthread")
add_compile_options(-Wall -Werror -Wfatal-errors -Wpedantic -Wextra -Wmissing-include-dirs -Wconversion -Wsign-conversion -pthread)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -march=native -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE DEBUG)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED OFF)
find_package(Boost 1.71 REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

include_directories(src)
include_directories(SYSTEM src/external)

add_subdirectory(src/rest)
add_subdirectory(src/ws)
add_subdirectory(src/utilws)
add_subdirectory(src/utilrest)
add_subdirectory(src/example)
add_subdirectory(src/bws)
add_subdirectory(src/brest)
