cmake_minimum_required(VERSION 3.9)

file(READ "SETUP-VERSION.txt" SETUP_VER)
string(STRIP "${SETUP_VER}" SETUP_VER)
file(READ "GCC-VERSION.txt" GCC_VER)
string(STRIP "${GCC_VER}" GCC_VER)

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
set_target_properties(tdminstall PROPERTIES PREFIX "")
target_link_libraries(
    tdminstall
    PRIVATE
    tdminstall_support
    "$ENV{XARC_LIB}"
    "${CMAKE_BINARY_DIR}/extlibs/xz/$ENV{XZ_LIB}"
    gdi32
    comctl32
)

add_executable(
    ctemplate
    ctemplate.cpp
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}/tinyxml2.cpp"
)
target_include_directories(
    ctemplate
    PRIVATE
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
)

add_custom_command(
    OUTPUT tdm-gcc-${GCC_VER}.exe
    COMMAND ${CMAKE_BINARY_DIR}/extlibs/$ENV{NSIS_DIRNAME}/makensis.exe -V2 -DOUTPUT_DIR=${CMAKE_BINARY_DIR}
        -DUPX_EXE=${CMAKE_BINARY_DIR}/extlibs/$ENV{UPX_DIRNAME}/upx.exe --
        ${CMAKE_CURRENT_SOURCE_DIR}/setup_full_tdm32.nsi
    DEPENDS
    ctemplate
    GCC-VERSION.txt
    inner-manifest-tdm32.txt
    main.nsh
    SETUP-VERSION.txt
    setup_full_tdm32.nsi
    tdminstall
)

add_custom_command(
    OUTPUT tdm64-gcc-${GCC_VER}.exe
    COMMAND ${CMAKE_BINARY_DIR}/extlibs/$ENV{NSIS_DIRNAME}/makensis.exe -V2 -DOUTPUT_DIR=${CMAKE_BINARY_DIR}
        -DUPX_EXE=${CMAKE_BINARY_DIR}/extlibs/$ENV{UPX_DIRNAME}/upx.exe --
        ${CMAKE_CURRENT_SOURCE_DIR}/setup_full_tdm64.nsi
    DEPENDS
    ctemplate
    GCC-VERSION.txt
    inner-manifest-tdm64.txt
    main.nsh
    SETUP-VERSION.txt
    setup_full_tdm64.nsi
    tdminstall
)

add_custom_command(
    OUTPUT tdm-gcc-webdl.exe
    COMMAND ${CMAKE_BINARY_DIR}/extlibs/$ENV{NSIS_DIRNAME}/makensis.exe -V2 -DOUTPUT_DIR=${CMAKE_BINARY_DIR}
        -DUPX_EXE=${CMAKE_BINARY_DIR}/extlibs/$ENV{UPX_DIRNAME}/upx.exe --
        ${CMAKE_CURRENT_SOURCE_DIR}/setup_webdl.nsi
    DEPENDS
    ctemplate
    main.nsh
    SETUP-VERSION.txt
    setup_webdl.nsi
    tdminstall
)

add_custom_target(
    installers ALL
    DEPENDS
    tdm-gcc-${GCC_VER}.exe
    tdm64-gcc-${GCC_VER}.exe
    tdm-gcc-webdl.exe
)
