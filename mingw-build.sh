#!/bin/sh

export CMAKE_PATH=/c/Program\ Files/CMake/bin
export GCC_PATH=/c/TDM-GCC-32/bin
export TINYXML2_URL="file://C:/Users/joeub/Downloads/tinyxml2-7.1.0.tar.gz"
export TINYXML2_DIRNAME="tinyxml2-7.1.0"
export XARC_INCLUDE=/c/Users/joeub/xarc/include
export XARC_LIB=/c/Users/joeub/build-xarc/libxarc.a
export XZ_URL="file://C:/Users/joeub/Downloads/xz-5.2.4-windows.7z"
export XZ_FILENAME="xz-5.2.4-windows.7z"
export XZ_LIB=bin_i686-sse2/liblzma.a
export NSIS_URL="file://C:/Users/joeub/Downloads/nsis-3.04.zip"
export NSIS_FILENAME="nsis-3.04.zip"
export NSIS_DIRNAME="nsis-3.04"
export UPX_URL="file://C:/Users/joeub/Downloads/upx-3.95-win32.zip"
export UPX_FILENAME="upx-3.95-win32.zip"
export UPX_DIRNAME="upx-3.95-win32"

export PATH=$CMAKE_PATH:$GCC_PATH:$PATH

mkdir -p extlibs && \
    ( \
        [ -d "extlibs/$TINYXML2_DIRNAME" ] || ( \
            cd extlibs && \
            curl -#L -o- "$TINYXML2_URL" | tar -zxf - \
        ) \
    ) && \
    ( \
        [ -d "extlibs/xz/include" ] || ( \
            mkdir -p extlibs/xz && \
            cd extlibs/xz && \
            curl -#JL -O "$XZ_URL" && \
            7z x $XZ_FILENAME \
        ) \
    ) && \
    ( \
        [ -d "extlibs/$NSIS_DIRNAME" ] || ( \
            cd extlibs && \
            curl -#JL -O "$NSIS_URL" && \
            7z x $NSIS_FILENAME \
        ) \
    ) && \
    ( \
        [ -d "extlibs/$UPX_DIRNAME" ] || ( \
            cd extlibs && \
            curl -#JL -O "$UPX_URL" && \
            7z x $UPX_FILENAME \
        ) \
    ) && \
    cmake -G "MSYS Makefiles" $(dirname $0) && \
    make
