@echo off
setlocal

set "WD=%__CD__%"

REM The PATH environment needs to include 7z.exe, mingw32-make.exe, and a
REM working MinGW or MinGW-w64 GCC installation.
set "PATH=C:\Program Files\7-Zip;C:\TDM-GCC-32-4.7.1\bin;%PATH%"
set "DISTRIB_BASE=C:\crossdev\gccmaster\distrib"

set "CMAKE_URL=file://C:/Users/John/Downloads/cmake-3.16.0-win32-x86.zip"
set "CMAKE_FILENAME=cmake-3.16.0-win32-x86.zip"
set "CMAKE_DIRNAME=cmake-3.16.0-win32-x86"
set "TINYXML2_URL=file://C:/Users/John/Downloads/tinyxml2-7.1.0.tar.gz"
set "TINYXML2_FILENAME=tinyxml2-7.1.0.tar.gz"
set "TINYXML2_DIRNAME=tinyxml2-7.1.0"
set "NSIS_URL=file://C:/Users/John/Downloads/nsis-3.04.zip"
set "NSIS_FILENAME=nsis-3.04.zip"
set "NSIS_DIRNAME=nsis-3.04"
set "UPX_URL=file://C:/Users/John/Downloads/upx-3.95-win32.zip"
set "UPX_FILENAME=upx-3.95-win32.zip"
set "UPX_DIRNAME=upx-3.95-win32"
set "XARC_INCLUDE=C:/crossdev/xarc/include"
set "XARC_LIB=C:/crossdev/xarc/build-cmd/libxarc.a"
set "LUA_URL=file://C:/Users/John/Downloads/lua-5.3.5.tar.gz"
set "LUA_FILENAME=lua-5.3.5.tar.gz"
set "LUA_DIRNAME=lua-5.3.5"

set "PATH=%WD%extlibs\%CMAKE_DIRNAME%\bin;%PATH%"

if not exist "%WD%extlibs/" ( mkdir "%WD%extlibs/" || exit /b %ERRORLEVEL% )

REM CMake
if not exist "%WD%extlibs/%CMAKE_FILENAME%" (
    echo Downloading CMAKE
    pushd "%WD%extlibs/"
    curl -#JLO "%CMAKE_URL%" || exit /b %ERRORLEVEL%
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
    curl -#JLO "%TINYXML2_URL%" || exit /b %ERRORLEVEL%
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
    curl -#JLO "%NSIS_URL%" || exit /b %ERRORLEVEL%
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
    curl -#JLO "%UPX_URL%" || exit /b %ERRORLEVEL%
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
    curl -#JLO "%LUA_URL%" || exit /b %ERRORLEVEL%
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
    popd
)

if not exist "%WD%Makefile" (
    echo Running CMAKE
    cmake.exe -G "MinGW Makefiles" "%~dp0" || exit /b %ERRORLEVEL%
)

set VERBOSE=1
mingw32-make.exe || exit /b %ERRORLEVEL%

exit /b 0
