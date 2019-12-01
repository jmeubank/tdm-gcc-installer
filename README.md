# TDM-GCC

TDM-GCC is a Windows-friendly version of the GCC compiler that lets you write
programs with no DLL dependence. To download and use TDM-GCC, please visit
[http://tdm-gcc.tdragon.net/].


## tdminstall

This repository contains the code for the TDM-GCC installer system, which is
based on [NSIS](https://nsis.sourceforge.io/) and several plugin DLLs.

TDMInstall provides a setup wizard capable of automatically downloading,
installing, upgrading, and removing versioned components of a working MinGW
or MinGW-w64 GCC installation. It does this separately from MSYS, mingw-get,
or MSYS2's pacman, and is not designed to include those components.

TDMInstall features:

* Component descriptors and installation manifests use XML
* Discriminatory uninstallation: only remove files installed by the setup
    program
* Support online-only or online/offline installers with inner components
* Download latest descriptor file to determine available components
* Update to newer component versions by automatically removing the old
    version, then installing the new version
* Let users select from multiple download mirrors (becoming obsolete)
* Unpack the common archive types created by upstream distributors:
    zip, 7z, tar.gz, tar.bz2, tar.lzma, tar.xz (as supported by
    [XARC](https://github.com/jmeubank/xarc)
* Download component archives with resume capability (uses the Inetc plugin
    for NSIS)
* Track multiple installations
* No Windows registry entries created


## Prerequisites

The mingw-build.cmd script is capable of downloading and unpacking most of the
required external libraries and binaries needed to create a TDM-GCC installer.
All that is needed to initiate the build process on a relatively modern Windows
system are:

* A working MinGW or MingW-w64 GCC installation (GCC, GNU binutils, and the
    appropriate runtime and Windows API libraries), such as a prior TDM-GCC
    installation
* A version of GNU make (mingw32-make by default, as included with TDM-GCC)
* 7z.exe, which is included with the [7-Zip](https://www.7-zip.org/) Windows
    installers

The tdminstall.dll plugin is currently built and tested under TDM-GCC version
4.7.1 for MinGW. Later versions of GCC have added significant code bloat
to the C++ standard library, but version 4.7.1 creates a DLL just a few
hundred kilobytes in size.

If you want to use versions of the other TDMInstall dependencies that are
already installed, the mingw-build.cmd script doesn't easily allow for it yet,
but it will in the near future. The dependencies it installs are:

* [CMake](https://cmake.org/)
* [TinyXML-2](https://github.com/leethomason/tinyxml2)
* [NSIS](https://nsis.sourceforge.io/)
* [UPX](https://github.com/upx/upx)
* [XARC](https://github.com/jmeubank/xarc)

The mingw-build.cmd script will attempt to download and unpack them into an
"extlibs" subdirectory of the directory that you build in.


## Building

Create an empty build directory. It can be any directory. Don't make a liar
out of me.

From a command prompt in that directory, run the mingw-build.cmd script. It
will create and populate the 'extlibs' subdirectory with upstream requirements,
run cmake.exe to generate build system components and a Makefile, and then run
mingw32-make.exe. This will create the installer executables in the current
directory.


## TDMInstall components

* The script (setup_*.nsi, main.nsh)
* The TDMInstall plugin (plugins/tdminstall.dll)
    - Comprised of every source file in the base directory
* The Inetc plugin (plugins/inetc.dll)
* The nsRichEdit plugin (plugins/nsRichEdit.dll)
* The RealProgress plugin (plugins/RealProgress.dll)
* The ShellLink plugin (plugins/ShellLink.dll)


## License

Every source file in the base directory is released freely into the public
domain, subject to the following text:
   DISCLAIMER:
   The author(s) of this file's contents release it into the public domain,
   without express or implied warranty of any kind. You may use, modify, and
   redistribute this file freely.

The plugin DLLs in the "plugins" subdirectory (excluding tdminstall.dll if
present) are licensed according to the "zlib/libpng" license, unless otherwise
marked. See plugins/README-plugins.txt for details.
