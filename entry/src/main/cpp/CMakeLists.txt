# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(hi_git)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

add_definitions(-DLOG_DOMAIN=0x0000)
add_definitions(-DLOG_TAG=\"appTag\")

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(src_files
    src/global.cpp
    src/core.cpp
    src/handler.cpp
    src/repo_manager.cpp
    src/ssh_manager.cpp
    utils/utils.hpp
    utils/raw.hpp
    napi_init.cpp
)

add_library(entry SHARED ${src_files})
target_include_directories(entry PUBLIC include)

# third party
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(entry PUBLIC nlohmann_json::nlohmann_json)

# system library
find_library(hilog-lib hilog_ndk.z REQUIRED)
find_library(napi-lib ace_napi.z REQUIRED)
find_library(rawfile-lib rawfile.z REQUIRED)
find_library(z-lib native_window REQUIRED)

set(sample_link_library
    ${hilog-lib}
    ${rawfile-lib}
    ${napi-lib}
    ${z-lib}
    libc++.a
    z
    libohfileio.so
    librawfile.z.so
)
target_link_libraries(entry PUBLIC ${sample_link_library})

# libgit2 and mbedtls
target_link_directories(entry PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party/libgit2/lib
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/lib
    ${CMAKE_SOURCE_DIR}/third_party/libssh2/lib
)
include_directories(entry PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party/libgit2/include
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include
    ${CMAKE_SOURCE_DIR}/third_party/libssh2/include
)
set(LIBS
    git2
    ssh2
    mbedcrypto
    mbedtls
    mbedx509
)
target_link_libraries(entry PUBLIC ${LIBS})