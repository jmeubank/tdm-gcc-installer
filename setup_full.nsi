; -- setup.nsi --
; Created: JohnE, 2008-03-15


; DISCLAIMER:
; The author(s) of this file's contents release it into the public domain,
; without express or implied warranty of any kind. You may use, modify, and
; redistribute this file freely.


!define INNER_COMPONENTS "inner-manifest.txt"

!define SETUP_VER "1.1004.0"
OutFile "output\tdm-mingw-1.1004.0gcc4.5.0.exe"

!packhdr "exehead.tmp" 'upx --best exehead.tmp'

!define SHORTNAME "TDM/MinGW"
!define DEF_INST_DIR "\MinGW"
!define INNER_MANIFEST "inner-manifest.txt"
!define LOCAL_NET_MANIFEST "net-manifest.txt"
!define NET_MANIFEST_URL "http://www.tdragon.net/tdminst/net-manifest.txt"
!define APPDATA_SUBFOLDER "MinGW"
!define STARTMENU_ENTRY "MinGW"
!define UNINSTKEY "TDM-GCC"
!define READMEFILE "README-gcc-tdm.txt"
!define INFOURL "http://www.tdragon.net/recentgcc/"
!define UPDATEURL "http://www.tdragon.net/recentgcc/"
!define PUBLISHER "TDM"

!include "main.nsh"
