cmake_minimum_required(VERSION 3.9)

file(READ "SETUP-VERSION.txt" SETUP_VER)
string(STRIP "${SETUP_VER}" SETUP_VER)
file(READ "GCC-VERSION.txt" GCC_VER)
string(STRIP "${GCC_VER}" GCC_VER)

project(tdm-gcc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=gnu++11 -m32")
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -F pe-i386")
#set(CMAKE_AR "gcc-ar")
#set(CMAKE_RANLIB "gcc-ranlib")

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
target_compile_options(tdminstall_support PRIVATE -Os -flto)

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
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LUA_DIRNAME}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_options(tdminstall PRIVATE -Os -flto)
target_link_options(tdminstall PRIVATE -s -Os -flto)
set_target_properties(tdminstall PROPERTIES PREFIX "")
target_link_libraries(
    tdminstall
    PRIVATE
    tdminstall_support
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LUA_DIRNAME}/src/liblua.a"
    "$ENV{XARC_LIB}"
    gdi32
    comctl32
)

add_executable(
    ctemplate
    ctemplate.cpp
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}/tinyxml2.cpp"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{MINILIBS_DIRNAME}/regexp.c"
)
target_include_directories(
    ctemplate
    PRIVATE
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{MINILIBS_DIRNAME}"
)

add_executable(
    prune_manifest
    prune_manifest.cc
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}/tinyxml2.cpp"
)
target_include_directories(
    prune_manifest
    PRIVATE
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{TINYXML2_DIRNAME}"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/inner-manifest-tdm32.txt
    COMMAND echo $ENV{DISTRIB_BASE}/$ENV{NET_MANIFEST_FILE}>${CMAKE_BINARY_DIR}/inner-manifest-tdm32.template.txt
    COMMAND echo tdm32>>${CMAKE_BINARY_DIR}/inner-manifest-tdm32.template.txt
    COMMAND echo tdmrec>>${CMAKE_BINARY_DIR}/inner-manifest-tdm32.template.txt
    COMMAND prune_manifest.exe >${CMAKE_CURRENT_SOURCE_DIR}/inner-manifest-tdm32.txt <${CMAKE_BINARY_DIR}/inner-manifest-tdm32.template.txt
    DEPENDS
    prune_manifest
    $ENV{DISTRIB_BASE}/$ENV{NET_MANIFEST_FILE}
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/inner-manifest-tdm64.txt
    COMMAND echo $ENV{DISTRIB_BASE}/$ENV{NET_MANIFEST_FILE}>${CMAKE_BINARY_DIR}/inner-manifest-tdm64.template.txt
    COMMAND echo tdm64>>${CMAKE_BINARY_DIR}/inner-manifest-tdm64.template.txt
    COMMAND echo tdmrec>>${CMAKE_BINARY_DIR}/inner-manifest-tdm64.template.txt
    COMMAND prune_manifest.exe >${CMAKE_CURRENT_SOURCE_DIR}/inner-manifest-tdm64.txt <${CMAKE_BINARY_DIR}/inner-manifest-tdm64.template.txt
    DEPENDS
    prune_manifest
    $ENV{DISTRIB_BASE}/$ENV{NET_MANIFEST_FILE}
)

add_custom_command(
    OUTPUT tdm-gcc-${GCC_VER}.exe
    COMMAND ${CMAKE_BINARY_DIR}/extlibs/$ENV{NSIS_DIRNAME}/makensis.exe -V2 -DOUTPUT_DIR=${CMAKE_BINARY_DIR}
        -DUPX_EXE=${CMAKE_BINARY_DIR}/extlibs/$ENV{UPX_DIRNAME}/upx.exe
        -DARCHIVES_DIR=$ENV{DISTRIB_BASE}\\tdm32-sjlj --
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
        -DUPX_EXE=${CMAKE_BINARY_DIR}/extlibs/$ENV{UPX_DIRNAME}/upx.exe
        -DARCHIVES_DIR=$ENV{DISTRIB_BASE}\\tdm64-seh --
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
