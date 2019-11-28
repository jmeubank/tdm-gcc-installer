#!/bin/sh

export PATH=/c/Program\ Files/CMake/bin:/c/TDM-GCC-32/bin:$PATH
export TINYXML2_URL="file://C:/Users/joeub/Downloads/tinyxml2-7.1.0.tar.gz"
export TINYXML2_DIRNAME="tinyxml2-7.1.0"
export XARC_INCLUDE=/c/Users/joeub/xarc/include
export XARC_LIB=/c/Users/joeub/build-xarc/libxarc.a
export XZ_URL="file://C:/Users/joeub/Downloads/xz-5.2.4-windows.7z"
export XZ_FILENAME="xz-5.2.4-windows.7z"
export XZ_LIB=bin_i686-sse2/liblzma.a

mkdir -p extlibs && \
    ( \
        [ -d "extlibs/$TINYXML2_DIRNAME" ] || ( \
            mkdir -p extlibs && \
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
    cmake -G "MSYS Makefiles" $(dirname $0) && \
    make
