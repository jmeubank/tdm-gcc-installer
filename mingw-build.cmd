@echo off
setlocal

set "WD=%__CD__%"

REM the PATH environment needs to include 7z.exe, mingw32-make.exe and a working MinGW or MinGW-w64 GCC
REM installation.
set "PATH=C:\Program Files\7-Zip;C:\TDM-GCC-32\bin;%PATH%"

set "CMAKE_URL=file://C:/Users/joeub/Downloads/cmake-3.16.0-win32-x86.zip"
set "CMAKE_FILENAME=cmake-3.16.0-win32-x86.zip"
set "CMAKE_DIRNAME=cmake-3.16.0-win32-x86"
set "TINYXML2_URL=file://C:/Users/joeub/Downloads/tinyxml2-7.1.0.tar.gz"
set "TINYXML2_FILENAME=tinyxml2-7.1.0.tar.gz"
set "TINYXML2_DIRNAME=tinyxml2-7.1.0"
set "NSIS_URL=file://C:/Users/joeub/Downloads/nsis-3.04.zip"
set "NSIS_FILENAME=nsis-3.04.zip"
set "NSIS_DIRNAME=nsis-3.04"
set "UPX_URL=file://C:/Users/joeub/Downloads/upx-3.95-win32.zip"
set "UPX_FILENAME=upx-3.95-win32.zip"
set "UPX_DIRNAME=upx-3.95-win32"
set "XARC_INCLUDE=C:/Users/joeub/xarc/include"
set "XARC_LIB=C:/Users/joeub/xarc/build-cmd/libxarc.a"

set "PATH=%WD%extlibs\%CMAKE_DIRNAME%\bin;%PATH%"

if not exist "%WD%extlibs/" ( mkdir "%WD%extlibs" || exit /b %ERRORLEVEL% )
if not exist "%WD%extlibs/%CMAKE_DIRNAME%/" (
    echo Downloading and extracting CMAKE
    pushd "%WD%/extlibs"
    curl -#JLO "%CMAKE_URL%" || exit /b %ERRORLEVEL%
    7z.exe x -y "%CMAKE_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%TINYXML2_DIRNAME%/" (
    echo Downloading and extracting TINYXML2
    pushd "%WD%/extlibs"
    curl -#JLO "%TINYXML2_URL%" || exit /b %ERRORLEVEL%
    ( 7z.exe x -so -y "%TINYXML2_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%NSIS_DIRNAME%/" (
    echo Downloading and extracting NSIS
    pushd "%WD%/extlibs"
    curl -#JLO "%NSIS_URL%" || exit /b %ERRORLEVEL%
    7z.exe x -y "%NSIS_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%UPX_DIRNAME%/" (
    echo Downloading and extracting UPX
    pushd "%WD%/extlibs"
    curl -#JLO "%UPX_URL%" || exit /b %ERRORLEVEL%
    7z.exe x -y "%UPX_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)

if not exist "%WD%Makefile" ( cmake.exe -G "MinGW Makefiles" "%~dp0" || exit /b %ERRORLEVEL% )

set VERBOSE=1
mingw32-make.exe || exit /b %ERRORLEVEL%

exit /b 0
