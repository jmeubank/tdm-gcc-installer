# Makefile for TDMInstall
# Created: JohnE, 2008-07-28


XARC_SRC = ../xarc
XARC_LIB = ../build-xarc
XZ_LIB = ../build-xarc/xz-prefix/src/xz/bin_i686-sse2

TDM_GCC_VER := $(strip $(shell type GCC-VERSION.txt))

SETUPEXEFULL32 = output\tdm-gcc-$(TDM_GCC_VER).exe
SETUPEXEFULL64 = output\tdm64-gcc-$(TDM_GCC_VER).exe
SETUPEXEWEBDL = output\tdm-gcc-webdl.exe
TDMINSTDLL = plugins\tdminstall.dll


CC = gcc
CFLAGS = -Wall -Os -I. -m32
#CFLAGS = -Wall -g -I. -m32
CXX = g++
CXXFLAGS = -Wall -Os -I. -m32 -std=gnu++11
#CXXFLAGS = -Wall -g -I. -m32
LD = g++
LDFLAGS = -s -m32 -std=gnu++11
#LDFLAGS = -m32
RC = windres
RCFLAGS = -F pe-i386

CFLAGS += -I$(XARC_SRC)/include
CXXFLAGS += -I$(XARC_SRC)/include
LDFLAGS += -L$(XARC_LIB) -L$(XZ_LIB)

.PHONY: all clean cleanall

all: $(SETUPEXEFULL32) $(SETUPEXEFULL64) $(SETUPEXEWEBDL)

clean:
	if exist $(TDMINSTDLL) del $(TDMINSTDLL)
	if exist output\*.exe del output\*.exe
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
  archive.o \
  componentstree.o \
  install_manifest.o \
  nsis_interface.o \
  tdminst_res.o \
  tdminstall.o

componentstree.o tdminstall.o: setup_version.h

SETUP_VER := $(strip $(shell type SETUP-VERSION.txt))
setup_version.h: SETUP-VERSION.txt
	echo #define STR_SETUP_VER "$(SETUP_VER)" >setup_version.h

TDMINSTDLL_ADDL_DEPENDS = \
 componentstree.hpp \
 config.h \
 install_manifest.hpp \
 nsis_interface.hpp \
 ref.hpp \
 tdminst_res.h

*.o: $(TDMINSTDLL_ADDL_DEPENDS)

$(TDMINSTDLL): $(TDMINSTDLLSOURCES) $(TDMINSTDLL_ADDL_DEPENDS)
	$(LD) -shared -Wl,--dll $(LDFLAGS) -o $(TDMINSTDLL) $(TDMINSTDLLSOURCES) \
	 -lxarc -llzma -lgdi32 -lcomctl32

tdminst_res.o: tdminst_res.rc tdminst_res.h
	$(RC) $(RCFLAGS) tdminst_res.rc tdminst_res.o
