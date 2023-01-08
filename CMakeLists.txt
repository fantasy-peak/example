cmake_minimum_required(VERSION 3.13.0)

project(example)

set(CMAKE_CXX_STANDARD 23)

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
set(XREPO_BOOTSTRAP_XMAKE ON)
set(XMAKE_RELEASE_LATEST v2.7.5)
set(XREPO_BUILD_PARALLEL_JOBS 12)

include(${CMAKE_BINARY_DIR}/xrepo.cmake)
if(NOT EXISTS "$ENV{HOME}/.xmake/repositories/my_private_repo/xmake.lua")
    message(STATUS "add my_private_repo")
    execute_process(COMMAND xmake repo -a -g my_private_repo https://github.com/fantasy-peak/xmake-repo.git)
endif()

#----------------------------------------------------------------------------------

xrepo_package("fmt 8.1.1")
# find_package(fmt REQUIRED)

xrepo_package("spdlog v1.9.2")
# find_package(spdlog REQUIRED)

xrepo_package("nlohmann_json v3.10.5")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/d/drogon/xmake.lua)
xrepo_package("drogon v1.8.2" DEPS "trantor" "jsoncpp 1.9.5" "zlib v1.2.12" "brotli 1.0.9")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/a/aws-lambda-cpp/xmake.lua)
xrepo_package("aws-lambda-cpp" DEPS "openssl" "libcurl")
find_package(aws-lambda-runtime REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/a/aws-sdk-cpp/xmake.lua)
xrepo_package("aws-sdk-cpp" DEPS "openssl" "libcurl" "c-ares")
find_package(AWSSDK COMPONENTS core)
find_package(AWSSDK COMPONENTS dynamodb)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/f/folly/xmake.lua)
xrepo_package("folly 2022.05.16" DEPS "conan::libiberty/9.1.0" "boost 1.79.0" "openssl" "libevent 2.1.12" "gflags v2.2.2" "glog v0.5.0" "fmt 8.1.1" "zlib v1.2.12" "double-conversion v3.1.5" "bzip2 1.0.8" "lz4 v1.9.3" "zstd v1.5.2")
# find_package(folly REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/r/redis-plus-plus/xmake.lua)
xrepo_package("redis-plus-plus" DEPS "hiredis" "boost 1.79.0")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/g/grpc/xmake.lua)
xrepo_package("grpc" DEPS "c-ares 1.17.1" "protobuf-cpp 3.19.4" "re2 2022.02.01" "openssl" "zlib v1.2.12")
find_package(gRPC CONFIG REQUIRED)

xrepo_package("sqlpp11" CONFIGS mariadb_connector=true)
# find_package(Sqlpp11 CONFIG REQUIRED)

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/b/boost/xmake.lua)
xrepo_package("boost 1.79.0" CONFIGS "context=true,filesystem=true,program_options=true,regex=true,system=true,thread=true")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/y/yaml_cpp_struct/xmake.lua)
xrepo_package("yaml_cpp_struct v1.0.0" DEPS "magic_enum" "visit_struct" "yaml-cpp")

set(XREPO_XMAKEFILE ${CMAKE_CURRENT_SOURCE_DIR}/repo/packages/l/libmysql/xmake.lua)
xrepo_package("libmysql 0.2.0" DEPS "boost 1.79.0")

#-----------------------------------------------------------------------------------

include_directories(include)

add_executable(test
    src/test.cpp
    src/helloworld.pb.cc
    src/helloworld.grpc.pb.cc
)
xrepo_target_packages(test PUBLIC sqlpp11 spdlog nlohmann_json aws-lambda-runtime aws-sdk-cpp folly drogon redis-plus-plus grpc NO_LINK_LIBRARIES)
target_link_libraries(test PUBLIC gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++
    mysqlclient
    AWS::aws-lambda-runtime
    aws-cpp-sdk-dynamodb curl
    ${AWSSDK_LINK_LIBRARIES}
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
    aws-cpp-sdk-dynamodb curl
    ${AWSSDK_LINK_LIBRARIES}
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

add_executable(test_boost src/test_boost.cpp)
xrepo_target_packages(test_boost PUBLIC spdlog boost NO_LINK_LIBRARIES)
target_link_libraries(test_boost PUBLIC
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    pthread dl
)

add_executable(test_yaml_cpp_struct src/test_yaml_cpp_struct.cpp)
xrepo_target_packages(test_yaml_cpp_struct PUBLIC spdlog yaml_cpp_struct NO_LINK_LIBRARIES)
target_link_libraries(test_yaml_cpp_struct PUBLIC yaml-cpp)

add_executable(test_boost_mysql src/test_boost_mysql.cpp)
xrepo_target_packages(test_boost_mysql PUBLIC spdlog libmysql folly NO_LINK_LIBRARIES)
target_link_libraries(test_boost_mysql PUBLIC
    folly glog gflags double-conversion zstd lz4 event event_core event_extra iberty event_openssl event_pthreads fmt
    boost_context-mt boost_program_options-mt boost_system-mt boost_thread-mt boost_regex-mt boost_filesystem-mt boost_atomic-mt
    ssl crypto pthread dl
)
