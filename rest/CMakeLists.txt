cmake_minimum_required(VERSION 3.28)
project(webvplayer VERSION 0.0.1)
include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

FetchContent_Declare(Crow
	GIT_REPOSITORY "https://github.com/CrowCpp/Crow.git"
	GIT_TAG "v1.2.0"
)

find_library(CMAKE_MPV NAMES mpv)

FetchContent_MakeAvailable(Crow)

add_executable(webvplayer)
target_compile_features(webvplayer PUBLIC cxx_std_20)
target_link_libraries(webvplayer ${CMAKE_MPV})
target_include_directories(webvplayer PUBLIC
	include
	${crow_SOURCE_DIR}/include
)
add_subdirectory(src)
