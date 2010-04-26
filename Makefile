# Makefile for TDMInstall
# Created: JohnE, 2008-07-28
#
# Tested under MinGW GCC 3.4.5 and (unofficial) 4.3.2


SETUPEXEFULL = output/tdm-mingw-1.1004.0gcc4.5.0.exe
SETUPEXEWEBDL = output/tdm-mingw-1.1004.0webdl.exe
TDMINSTDLL = plugins/tdminstall.dll


CC = gcc
CFLAGS = -Wall -Os -I. -Ibzip2 -Izlib -m32
CXX = g++
CXXFLAGS = -Wall -Os -I. -Ibzip2 -Izlib -m32
LD = g++
LDFLAGS = -s -m32
RC = windres
RCFLAGS = -F pe-i386


.PHONY: all

all: $(SETUPEXEFULL) $(SETUPEXEWEBDL)

$(SETUPEXEFULL): setup_full.nsi main.nsh ctemplate.exe $(TDMINSTDLL)
	makensis /V2 setup_full.nsi

$(SETUPEXEWEBDL): setup_webdl.nsi main.nsh ctemplate.exe $(TDMINSTDLL)
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
  7z/Util/7z/7zAlloc.o \
  7z/7zBuf.o \
  7z/7zCrc.o \
  7z/7zCrcOpt.o \
  7z/7zDec.o \
  7z/7zFile.o \
  7z/7zIn.o \
  7z/7zStream.o \
  7z/Bcj2.o \
  7z/Bra86.o \
  7z/CpuArch.o \
  7z/LzmaDec.o \
  7z/Lzma2Dec.o \
  bzip2/bzlib.o \
  bzip2/crctable.o \
  bzip2/decompress.o \
  bzip2/huffman.o \
  bzip2/randtable.o \
  zlib/contrib/minizip/ioapi.o \
  zlib/contrib/minizip/unzip.o \
  zlib/adler32.o \
  zlib/crc32.o \
  zlib/deflate.o \
  zlib/gzio.o \
  zlib/inffast.o \
  zlib/inflate.o \
  zlib/inftrees.o \
  zlib/trees.o \
  zlib/zutil.o \
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

$(TDMINSTDLL): $(TDMINSTDLLSOURCES)
	$(LD) -shared -Wl,--dll $(LDFLAGS) -o $(TDMINSTDLL) $(TDMINSTDLLSOURCES) \
	 -lgdi32 -lcomctl32

tdminst_res.o: tdminst_res.rc tdminst_res.h
	$(RC) $(RCFLAGS) tdminst_res.rc tdminst_res.o
