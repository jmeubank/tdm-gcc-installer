README - TDMInstall

TDMInstall is an aggregate work based on the NSIS installer-creation system,
comprised of a script and several plugin DLLs. The end goal of TDMInstall is to
provide a setup wizard capable of automatically downloading, installing,
upgrading, and removing versioned components of a software package.

Some of TDMInstall's features:
 * Component descriptors and installation manifests use XML
 * Discriminatory uninstallation: only remove files installed by the setup
     program
 * Support online-only or online/offline installers with inner components
 * Download latest descriptor file to determine available components
 * Update to newer component versions by automatically removing the old version,
     then installing the new version
 * Let users select from multiple download mirrors
 * Unpack the common archive types supported by XARC, maintaining original file
     attributes: zip, 7z, tar.gz, tar.bz2, tar.lzma, tar.xz
 * Download component archives with resume capability (uses the Inetc plugin for
     NSIS)
 * Track multiple installations
 * No Windows registry entries created


=====
Prerequisites
=====

TDMInstall is currently built and tested using NSIS 2.46; you must have a
similarly-numbered version of NSIS installed. (nsDialogs and MUI2 are required.)

tdminstall.dll is currently built and tested under GCC version 4.7.1 for MinGW,
and the included Makefile is designed for MinGW/GCC. It is recommended that you
use the 3.4 series or later.

UPX (<http://upx.sourceforge.net/>) can be used to achieve a smaller size for
the final executable. For upx.exe to be found, add its directory to PATH or
place it in the base TDMInstall directory.


=====
Building
=====


=====
TDMInstall's Components
=====

 * The script (setup_*.nsi, main.nsh)
 * The TDMInstall plugin (plugins/tdminstall.dll)
    - Comprised of every source file in the base directory, as well as the
      source files in the "boost" subdirectory.
 * The Inetc plugin (plugins/inetc.dll)
 * The RealProgress plugin (plugins/RealProgress.dll)
 * The ShellLink plugin (plugins/ShellLink.dll)


=====
Licensing
=====

Every source file in the base directory is released freely into the public
domain, subject to the following text:
   DISCLAIMER:
   The author(s) of this file's contents release it into the public domain,
   without express or implied warranty of any kind. You may use, modify, and
   redistribute this file freely.

The source files in the "boost" subdirectory are licensed according to Boost's
license. See boost/LICENSE_1_0.txt for details.

The plugin DLLs in the "plugins" subdirectory (excluding tdminstall.dll if
present) are licensed according to the "zlib/libpng" license, unless otherwise
marked. See plugins/README-plugins.txt for details.
