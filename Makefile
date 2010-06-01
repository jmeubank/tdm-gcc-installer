# Makefile for TDMInstall
# Created: JohnE, 2008-07-28


TDM_GCC_VER := $(strip $(shell type GCC-VERSION.txt))

SETUPEXEFULL32 = output\tdm-gcc-$(TDM_GCC_VER).exe
SETUPEXEFULL64 = output\tdm64-gcc-$(TDM_GCC_VER).exe
SETUPEXEWEBDL = output\tdm-gcc-webdl.exe
TDMINSTDLL = plugins\tdminstall.dll


CC = gcc
CFLAGS = -Wall -Os -I. -Ibzip2 -Izlib -m32
CXX = g++
CXXFLAGS = -Wall -Os -I. -Ibzip2 -Izlib -m32
LD = g++
LDFLAGS = -s -m32
RC = windres
RCFLAGS = -F pe-i386


.PHONY: all clean cleanall

all: $(SETUPEXEFULL32) $(SETUPEXEFULL64) $(SETUPEXEWEBDL)

clean:
	if exist $(TDMINSTDLL) del $(TDMINSTDLL)
	if exist 7z\Util\7z\*.o del 7z\Util\7z\*.o
	if exist 7z\*.o del 7z\*.o
	if exist bzip2\*.o del bzip2\*.o
	if exist output\*.exe del output\*.exe
	if exist zlib\contrib\minizip\*.o del zlib\contrib\minizip\*.o
	if exist zlib\*.o del zlib\*.o
	if exist *.o del *.o
	if exist setup_version.h del setup_version.h
	if exist ctemplate.exe del ctemplate.exe

cleanall: clean

COMMON_DEPENDS = \
 $(TDMINSTDLL) \
 ctemplate.exe \
 main.nsh \
 SETUP-VERSION.txt

$(SETUPEXEFULL32): setup_full_tdm32.nsi inner-manifest-tdm32.txt \
 GCC-VERSION.txt $(COMMON_DEPENDS)
	makensis /V2 setup_full_tdm32.nsi

$(SETUPEXEFULL64): setup_full_tdm64.nsi inner-manifest-tdm64.txt \
 GCC-VERSION.txt $(COMMON_DEPENDS)
	makensis /V2 setup_full_tdm64.nsi

$(SETUPEXEWEBDL): setup_webdl.nsi $(COMMON_DEPENDS)
	makensis /V2 setup_webdl.nsi

TINYXMLSOURCES = \
  tinystr.o \
  tinyxml.o \
  tinyxmlerror.o \
  tinyxmlparser.o

ctemplate.exe: ctemplate.o $(TINYXMLSOURCES)
	$(LD) $(LDFLAGS) -o ctemplate.exe ctemplate.o $(TINYXMLSOURCES)

TDMINSTDLLSOURCES = \
  $(TINYXMLSOURCES) \
  7z\Util\7z\7zAlloc.o \
  7z\7zBuf.o \
  7z\7zCrc.o \
  7z\7zCrcOpt.o \
  7z\7zDec.o \
  7z\7zFile.o \
  7z\7zIn.o \
  7z\7zStream.o \
  7z\Bcj2.o \
  7z\Bra86.o \
  7z\CpuArch.o \
  7z\LzmaDec.o \
  7z\Lzma2Dec.o \
  bzip2\bzlib.o \
  bzip2\crctable.o \
  bzip2\decompress.o \
  bzip2\huffman.o \
  bzip2\randtable.o \
  zlib\contrib\minizip\ioapi.o \
  zlib\contrib\minizip\unzip.o \
  zlib\adler32.o \
  zlib\crc32.o \
  zlib\deflate.o \
  zlib\gzio.o \
  zlib\inffast.o \
  zlib\inflate.o \
  zlib\inftrees.o \
  zlib\trees.o \
  zlib\zutil.o \
  archive.o \
  archive_7z.o \
  archive_base.o \
  archive_zip.o \
  componentstree.o \
  install_manifest.o \
  multiread.o \
  multiread_lzma.o \
  nsis_interface.o \
  tdminst_res.o \
  tdminstall.o \
  untgz.o

componentstree.o tdminstall.o: setup_version.h

SETUP_VER := $(strip $(shell type SETUP-VERSION.txt))
setup_version.h: SETUP-VERSION.txt
	echo #define STR_SETUP_VER "$(SETUP_VER)" >setup_version.h

TDMINSTDLL_ADDL_DEPENDS = \
 archive_base.h \
 componentstree.hpp \
 config.h \
 install_manifest.hpp \
 multiread.h \
 nsis_interface.hpp \
 ref.hpp \
 tdminst_res.h

$(TDMINSTDLL): $(TDMINSTDLLSOURCES) $(TDMINSTDLL_ADDL_DEPENDS)
	$(LD) -shared -Wl,--dll $(LDFLAGS) -o $(TDMINSTDLL) $(TDMINSTDLLSOURCES) \
	 -lgdi32 -lcomctl32

tdminst_res.o: tdminst_res.rc tdminst_res.h
	$(RC) $(RCFLAGS) tdminst_res.rc tdminst_res.o
