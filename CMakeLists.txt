cmake_minimum_required(VERSION 3.13.0)

project(xmake-example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fcoroutines")

if(NOT EXISTS "${CMAKE_BINARY_DIR}/xrepo.cmake")
    message(STATUS "Downloading xrepo.cmake from https://github.com/xmake-io/xrepo-cmake/")
    # mirror https://cdn.jsdelivr.net/gh/xmake-io/xrepo-cmake@main/xrepo.cmake
    file(DOWNLOAD "https://raw.githubusercontent.com/xmake-io/xrepo-cmake/main/xrepo.cmake"
                  "${CMAKE_BINARY_DIR}/xrepo.cmake"
                  TLS_VERIFY ON)
endif()
include(${CMAKE_BINARY_DIR}/xrepo.cmake)

#-----------------------------------------------------------------------------------
xrepo_package("fmt 8.1.1")
find_package(fmt REQUIRED)

xrepo_package("spdlog v1.9.2")
find_package(spdlog REQUIRED)

xrepo_package("nlohmann_json v3.10.5")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/f/folly/xmake.lua)
xrepo_package("folly 2022.04.11" DEPS "conan::libiberty/9.1.0" "boost 1.78.0" "openssl 1.1.1m" "libevent 2.1.12" "glog v0.5.0" "fmt" "zlib" "gflags v2.2.2" "double-conversion v3.1.5" "bzip2 1.0.8" "lz4 v1.9.3" "zstd v1.5.2")
find_package(folly REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/d/drogon/xmake.lua)
xrepo_package("drogon" DEPS "trantor" "jsoncpp" "zlib" "brotli")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/r/redis-plus-plus/xmake.lua)
xrepo_package("redis-plus-plus" DEPS "hiredis" "boost 1.78.0")
#-----------------------------------------------------------------------------------

include_directories(include)

add_executable(example-bin "")
target_sources(example-bin PRIVATE src/main.cpp)
# xrepo_target_packages(example-bin publi fmt spdlog nlohmann_json folly drogon trantor redis++)
xrepo_target_packages(example-bin PUBLIC fmt spdlog nlohmann_json folly drogon redis-plus-plus NO_LINK_LIBRARIES)
target_link_libraries(example-bin PUBLIC fmt 
    folly redis++ hiredis uv glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads 
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    drogon trantor jsoncpp brotlidec brotlienc brotlicommon ssl crypto cares uuid -pthread dl z
)


