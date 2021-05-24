@echo off
setlocal

set "WD=%__CD__%"

REM The PATH environment needs to include 7z.exe, mingw32-make.exe, and a
REM working MinGW or MinGW-w64 GCC installation.
set "PATH=C:\Program Files\7-Zip;C:\TDM-GCC-32-4.7.1\bin;%PATH%"
@REM set "PATH=C:\Program Files\7-Zip;C:\crossdev\test-toolchain-tdm64\bin;%PATH%"
set "DISTRIB_BASE=C:\crossdev\gccmaster\distrib"
set "NET_MANIFEST_FILE=net-manifest.txt.new.txt"

set "CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-win32-x86.zip"
set "CMAKE_FILENAME=cmake-3.17.3-win32-x86.zip"
set "CMAKE_DIRNAME=cmake-3.17.3-win32-x86"
set "TINYXML2_URL=https://github.com/leethomason/tinyxml2/archive/7.1.0.tar.gz"
set "TINYXML2_FILENAME=tinyxml2-7.1.0.tar.gz"
set "TINYXML2_DIRNAME=tinyxml2-7.1.0"
set "NSIS_URL=https://sourceforge.net/projects/nsis/files/NSIS%%203/3.04/nsis-3.04.zip/download"
set "NSIS_FILENAME=nsis-3.04.zip"
set "NSIS_DIRNAME=nsis-3.04"
set "UPX_URL=https://github.com/upx/upx/releases/download/v3.96/upx-3.96-win32.zip"
set "UPX_FILENAME=upx-3.96-win32.zip"
set "UPX_DIRNAME=upx-3.96-win32"
set "XARC_INCLUDE=C:/crossdev/xarc/include"
set "XARC_LIB=C:/crossdev/xarc/build-cmd/libxarc.a"
set "LUA_URL=https://www.lua.org/ftp/lua-5.3.5.tar.gz"
set "LUA_FILENAME=lua-5.3.5.tar.gz"
set "LUA_DIRNAME=lua-5.3.5"
set "MINILIBS_URL=https://github.com/ccxvii/minilibs/archive/master.zip"
set "MINILIBS_FILENAME=minilibs-master.zip"
set "MINILIBS_DIRNAME=minilibs-master"
set "WIX_URL=https://github.com/wixtoolset/wix3/releases/download/wix3112rtm/wix311-binaries.zip"
set "WIX_FILENAME=wix311-binaries.zip"
set "WIX_DIRNAME=wix-3.11.2-binaries"
set "TINYCURL_URL=tiny-curl-7.72.0.tar.gz"
set "TINYCURL_FILENAME=tiny-curl-7.72.0.tar.gz"
set "TINYCURL_DIRNAME=tiny-curl-7.72.0"

set "PATH=%WD%extlibs\%CMAKE_DIRNAME%\bin;%PATH%"

if not exist "%WD%extlibs/" ( mkdir "%WD%extlibs/" || exit /b %ERRORLEVEL% )

REM CMake
if not exist "%WD%extlibs/%CMAKE_FILENAME%" (
    echo Downloading CMAKE
    pushd "%WD%extlibs/"
    curl -#JL "%CMAKE_URL%" -o "%CMAKE_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%CMAKE_DIRNAME%/" (
    echo Unpacking CMAKE
    pushd "%WD%extlibs/"
    7z.exe x -y "%CMAKE_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)

REM TinyXML2
if not exist "%WD%extlibs/%TINYXML2_FILENAME%" (
    echo Downloading TINYXML2
    pushd "%WD%extlibs/"
    curl -#JL "%TINYXML2_URL%" -o "%TINYXML2_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%TINYXML2_DIRNAME%/" (
    echo Unpacking TINYXML2
    pushd "%WD%extlibs/"
    ( 7z.exe x -so -y "%TINYXML2_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)

REM NSIS
if not exist "%WD%extlibs/%NSIS_FILENAME%" (
    echo Downloading NSIS
    pushd "%WD%extlibs/"
    curl -#JL "%NSIS_URL%" -o "%NSIS_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%NSIS_DIRNAME%/" (
    echo Unpacking NSIS
    pushd "%WD%extlibs/"
    7z.exe x -y "%NSIS_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)

REM UPX
if not exist "%WD%extlibs/%UPX_FILENAME%" (
    echo Downloading UPX
    pushd "%WD%extlibs/"
    curl -#JL "%UPX_URL%" -o "%UPX_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%UPX_DIRNAME%/" (
    echo Unpacking UPX
    pushd "%WD%extlibs/"
    7z.exe x -y "%UPX_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)

REM Lua
if not exist "%WD%extlibs/%LUA_FILENAME%" (
    echo Downloading LUA
    pushd "%WD%extlibs/"
    curl -#JL "%LUA_URL%" -o "%LUA_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%LUA_DIRNAME%/" (
    echo Unpacking LUA
    pushd "%WD%extlibs/"
    ( 7z.exe x -so -y "%LUA_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%LUA_DIRNAME%/src/liblua.a" (
    echo Building LUA
    pushd "%WD%extlibs/%LUA_DIRNAME%/"
    mingw32-make.exe mingw CC="gcc -m32 -std=gnu99 -flto -fuse-linker-plugin" CFLAGS="-m32 -Os -flto" LDFLAGS="-m32 -Os -fuse-linker-plugin" || exit /b %ERRORLEVEL%
    @REM mingw32-make.exe mingw CC="gcc -m32 -std=gnu99" CFLAGS="-m32 -Os" LDFLAGS="-m32 -Os" || exit /b %ERRORLEVEL%
    popd
)

REM Minilibs
if not exist "%WD%extlibs/%MINILIBS_FILENAME%" (
    echo Downloading Minilibs
    pushd "%WD%extlibs/"
    curl -#JL "%MINILIBS_URL%" -o "%MINILIBS_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%MINILIBS_DIRNAME%/" (
    echo Unpacking Minilibs
    pushd "%WD%extlibs/"
    7z.exe x -y "%MINILIBS_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)

REM WolfSSL
REM ./configure --build=mingw32 --disable-shared --enable-static
@REM if not exist "%WD%extlibs/%WOLFSSL_FILENAME%" (
@REM     echo Downloading WolfSSL
@REM     pushd "%WD%extlibs/"
@REM     curl -#JL "%WOLFSSL_URL%" -o "%WOLFSSL_FILENAME%" || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%WOLFSSL_DIRNAME%/" (
@REM     echo Unpacking WolfSSL
@REM     pushd "%WD%extlibs/"
@REM     ( 7z.exe x -so -y "%WOLFSSL_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%WOLFSSL_DIRNAME%/Makefile" (
@REM     echo Configuring WolfSSL with CMake
@REM     pushd "%WD%extlibs/%WOLFSSL_DIRNAME%/"
@REM     cmake.exe -G "MinGW Makefiles" . -DBUILD_TESTS=NO || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%WOLFSSL_DIRNAME%/libwolfssl.a" (
@REM     echo Building WolfSSL
@REM     pushd "%WD%extlibs/%WOLFSSL_DIRNAME%/"
@REM     REM set VERBOSE=1
@REM     mingw32-make.exe || exit /b %ERRORLEVEL%
@REM     popd
@REM )

REM tiny-curl
REM +    export CFLAGS="-Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -flto"
REM +    export LDFLAGS="-Wl,-s -Wl,-Bsymbolic -Wl,--gc-sections"
REM ./configure \
REM     --disable-cookies \
REM     --disable-crypto-auth \
REM     --disable-dict \
REM     --disable-file \
REM     --disable-ftp \
REM     --disable-gopher \
REM     --disable-imap \
REM     --disable-ldap \
REM     --disable-pop3 \
REM     --disable-proxy \
REM     --disable-rtmp \
REM     --disable-rtsp \
REM     --disable-scp \
REM     --disable-sftp \
REM     --disable-smtp \
REM     --disable-telnet \
REM     --disable-tftp \
REM     --disable-unix-sockets \
REM     --disable-verbose \
REM     --disable-versioned-symbols \
REM     --disable-http-auth \
REM     --disable-doh \
REM     --disable-mime \
REM     --disable-dateparse \
REM     --disable-netrc \
REM     --disable-dnsshuffle \
REM     --disable-progress-meter \
REM     --enable-maintainer-mode \
REM     --enable-werror \
REM     --without-brotli \
REM     --without-gssapi \
REM     --without-libidn2 \
REM     --without-libmetalink \
REM     --without-libpsl \
REM     --without-librtmp \
REM     --without-libssh2 \
REM     --without-nghttp2 \
REM     --without-ntlm-auth \
REM     --without-ssl \
REM     --without-zlib \
REM     --with-wolfssl \
REM     --build=mingw32 \
REM     --disable-pthreads \
REM     --disable-shared \
REM     CPPFLAGS=-IC:/crossdev/src/wolfssl-4.5.0-stable \
REM     LDFLAGS=-LC:/crossdev/src/wolfssl-4.5.0-stable/src/.libs
@REM if not exist "%WD%extlibs/%TINYCURL_FILENAME%" (
@REM     echo Downloading tiny-curl
@REM     pushd "%WD%extlibs/"
@REM     curl -#JL "%TINYCURL_URL%" -o "%TINYCURL_FILENAME%" || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%TINYCURL_DIRNAME%/" (
@REM     echo Unpacking tiny-curl
@REM     pushd "%WD%extlibs/"
@REM     ( 7z.exe x -so -y "%TINYCURL_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%TINYCURL_DIRNAME%/build/Makefile" (
@REM     echo Configuring tiny-curl with CMake
@REM     if not exist "%WD%extlibs/%TINYCURL_DIRNAME%/build/" (
@REM         md "%WD%extlibs/%TINYCURL_DIRNAME%/build/" || exit /b %ERRORLEVEL%
@REM     )
@REM     pushd "%WD%extlibs/%TINYCURL_DIRNAME%/build/"
@REM     cmake.exe -G "MinGW Makefiles" .. -DCMAKE_USE_WOLFSSL=ON -DWolfSSL_INCLUDE_DIR=%WD%extlibs/%WOLFSSL_DIRNAME% -DWolfSSL_LIBRARY=%WD%extlibs/%WOLFSSL_DIRNAME%/libwolfssl.a || exit /b %ERRORLEVEL%
@REM     popd
@REM )
@REM if not exist "%WD%extlibs/%TINYCURL_DIRNAME%/build/libwolfssl.a" (
@REM     echo Building tiny-curl with mingw32-make / CMake
@REM     pushd "%WD%extlibs/%TINYCURL_DIRNAME%/build/"
@REM     REM set VERBOSE=1
@REM     mingw32-make.exe || exit /b %ERRORLEVEL%
@REM     popd
@REM )

if not exist "%WD%Makefile" (
    echo Generating MinGW Makefiles with CMAKE
    cmake.exe -G "MinGW Makefiles" "%~dp0" || exit /b %ERRORLEVEL%
)

echo Building tdminstall with mingw32-make / CMake
REM set VERBOSE=1
mingw32-make.exe || exit /b %ERRORLEVEL%

exit /b 0
