cmake_minimum_required(VERSION 3.13.0)

project(xmake-example)

set(CMAKE_CXX_STANDARD 20)

set(OPTFLAGS -gdwarf-2)
set(WARNINGS -Wall -Wno-missing-field-initializers -Wno-ignored-qualifiers -g)
add_compile_options(${OPTFLAGS} ${WARNINGS})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/xrepo.cmake")
    message(STATUS "Downloading xrepo.cmake from https://github.com/xmake-io/xrepo-cmake/")
    # mirror https://cdn.jsdelivr.net/gh/xmake-io/xrepo-cmake@main/xrepo.cmake
    file(DOWNLOAD "https://raw.githubusercontent.com/xmake-io/xrepo-cmake/main/xrepo.cmake"
                  "${CMAKE_BINARY_DIR}/xrepo.cmake"
                  TLS_VERIFY ON)
endif()

set(XREPO_PACKAGE_VERBOSE ON)
set(XREPO_BUILD_PARALLEL_JOBS 12)

include(${CMAKE_BINARY_DIR}/xrepo.cmake)

execute_process(COMMAND xmake repo -a -g my_private_repo https://github.com/fantasy-peak/xmake-repo.git)

#----------------------------------------------------------------------------------

xrepo_package("fmt 8.1.1")
# find_package(fmt REQUIRED)

xrepo_package("spdlog v1.9.2")
# find_package(spdlog REQUIRED)

xrepo_package("nlohmann_json v3.10.5")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/d/drogon/xmake.lua)
xrepo_package("drogon" DEPS "trantor" "jsoncpp 1.9.5" "zlib v1.2.12" "brotli 1.0.9")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/a/aws-lambda-cpp/xmake.lua)
xrepo_package("aws-lambda-cpp" DEPS "openssl 1.1.1n" "libcurl")
find_package(aws-lambda-runtime REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/a/aws-sdk-cpp/xmake.lua)
xrepo_package("aws-sdk-cpp" DEPS "openssl 1.1.1n" "libcurl" "c-ares")
find_package(AWSSDK COMPONENTS core)
find_package(AWSSDK COMPONENTS dynamodb)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/f/folly/xmake.lua)
xrepo_package("folly 2022.04.25" DEPS "conan::libiberty/9.1.0" "boost 1.78.0" "openssl 1.1.1n" "libevent 2.1.12" "gflags v2.2.2" "glog v0.5.0" "fmt 8.1.1" "zlib v1.2.12" "double-conversion v3.1.5" "bzip2 1.0.8" "lz4 v1.9.3" "zstd v1.5.2")
# find_package(folly REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/r/redis-plus-plus/xmake.lua)
xrepo_package("redis-plus-plus" DEPS "hiredis" "boost 1.78.0")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/g/grpc/xmake.lua)
xrepo_package("grpc" DEPS "c-ares 1.17.1" "protobuf-cpp 3.19.4" "re2 2022.02.01" "openssl 1.1.1n" "zlib v1.2.12")
find_package(gRPC CONFIG REQUIRED)

xrepo_package("sqlpp11" CONFIGS mariadb_connector=true)
# find_package(Sqlpp11 CONFIG REQUIRED)

#-----------------------------------------------------------------------------------

include_directories(include)

add_executable(example-bin "")
target_sources(example-bin PRIVATE 
    src/main.cpp
    src/helloworld.pb.cc
    src/helloworld.grpc.pb.cc
)

xrepo_target_packages(example-bin PUBLIC sqlpp11 spdlog nlohmann_json aws-lambda-runtime aws-sdk-cpp folly drogon redis-plus-plus grpc NO_LINK_LIBRARIES)
target_link_libraries(example-bin PUBLIC gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++
    mysqlclient
    AWS::aws-lambda-runtime
    ${AWSSDK_LINK_LIBRARIES}
    aws-cpp-sdk-dynamodb curl
    drogon trantor jsoncpp brotlidec brotlienc brotlicommon
    redis++ hiredis
    folly uv glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto cares uuid pthread dl
)

add_executable(test_folly src/test_folly.cpp)
xrepo_target_packages(test_folly PUBLIC fmt spdlog folly  NO_LINK_LIBRARIES)
target_link_libraries(test_folly PUBLIC
    folly glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto pthread dl
)

add_executable(test_drogon src/test_drogon.cpp)
xrepo_target_packages(test_drogon PUBLIC fmt spdlog folly drogon NO_LINK_LIBRARIES)
target_link_libraries(test_drogon PUBLIC
    drogon trantor jsoncpp brotlidec brotlienc brotlicommon z
    folly uv glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto cares uuid pthread dl
)

add_executable(test_redis++ src/test_redis++.cpp)
xrepo_target_packages(test_redis++ PUBLIC fmt spdlog folly redis-plus-plus NO_LINK_LIBRARIES)
target_link_libraries(test_redis++ PUBLIC
    redis++ hiredis
    folly uv glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto pthread dl
)

add_executable(test_aws_lambda src/test_aws_lambda.cpp)
xrepo_target_packages(test_aws_lambda PUBLIC nlohmann_json aws-lambda-runtime aws-sdk-cpp NO_LINK_LIBRARIES)
target_link_libraries(test_aws_lambda PUBLIC
    AWS::aws-lambda-runtime
    ${AWSSDK_LINK_LIBRARIES}
    aws-cpp-sdk-dynamodb curl
    ssl crypto pthread dl
)

add_executable(test_tcp_server src/test_tcp_server.cpp)
xrepo_target_packages(test_tcp_server PUBLIC spdlog folly NO_LINK_LIBRARIES)
target_link_libraries(test_tcp_server PUBLIC
    folly glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto pthread dl
)

add_executable(test_tcp_client src/test_tcp_client.cpp)
xrepo_target_packages(test_tcp_client PUBLIC spdlog folly NO_LINK_LIBRARIES)
target_link_libraries(test_tcp_client PUBLIC
    folly glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto pthread dl
)



