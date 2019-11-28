cmake_minimum_required(VERSION 3.9)

file(READ "SETUP-VERSION.txt" SETUP_VER)
string(STRIP "${SETUP_VER}" SETUP_VER)

project(tdm-gcc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -std=gnu++11 -fno-rtti")
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -F pe-i386")

add_library(
    tdminstall_support STATIC
    archive.cpp
    componentstree.cpp
    install_manifest.cpp
    nsis_interface.cpp
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}/tinyxml2.cpp"
)
target_include_directories(
    tdminstall_support
    PRIVATE
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "$ENV{XARC_INCLUDE}"
)

configure_file(setup_version.h.in setup_version.h @ONLY)
add_library(
    tdminstall SHARED
    tdminst_res.rc
    tdminstall.cpp
)
target_include_directories(
    tdminstall
    PRIVATE
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_options(tdminstall PRIVATE -s)
target_link_directories(tdminstall PRIVATE "${XZ_SOURCE_DIR}/bin_i686-sse2")
target_link_libraries(
    tdminstall
    PRIVATE
    tdminstall_support
    "$ENV{XARC_LIB}"
    "${CMAKE_BINARY_DIR}/extlibs/xz/$ENV{XZ_LIB}"
    gdi32
    comctl32
)
