cmake_minimum_required(VERSION 3.9)

file(READ "SETUP-VERSION.txt" SETUP_VER)
string(STRIP "${SETUP_VER}" SETUP_VER)

project(tdm-gcc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -std=gnu++11")
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -F pe-i386")

add_library(
    tdminstall SHARED
    archive.cpp
    componentstree.cpp
    install_manifest.cpp
    nsis_interface.cpp
    tdminst_res.rc
    tdminstall.cpp
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}/tinyxml2.cpp"
)
target_include_directories(
    tdminstall PUBLIC
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
    "$ENV{XARC_INCLUDE}"
)
target_link_options(tdminstall PUBLIC -s)
target_link_directories(tdminstall PUBLIC "${XZ_SOURCE_DIR}/bin_i686-sse2")
target_link_libraries(
    tdminstall PUBLIC
    "$ENV{XARC_LIB}"
    "${CMAKE_BINARY_DIR}/extlibs/xz/$ENV{XZ_LIB}"
    gdi32
    comctl32
)
