# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ole32

TARGET_BASE = 0x77a50000

TARGET_CFLAGS = \
 -Wall \
 -fno-builtin

# Compile definitions usage:
#    __USE_W32API - Compilation with w32api headers
#    __REACTOS__ - Compilation of Wine sources for ReactOS
#    _WIN32_IE=0x600 - Internet Explorer 6 compatible defintions
#    WINVER=0x501 - Windows XP definitions
#    __need_offsetof - Force definition of macro offsetof in stddef.h
#
DEFINES = \
	-D__USE_W32API \
	-D__REACTOS__ \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 \
	-DWINVER=0x501 \
	-DCOBJMACROS \
	-D__need_offsetof
	
TARGET_CFLAGS += \
	$(DEFINES) \
	-I$(PATH_TO_TOP)/include/wine \
	-Wno-unused

TARGET_RCFLAGS += -D__REACTOS__	-D_WIN32_IE=0x600 -D_WIN32_WINNT=0x501

TARGET_SDKLIBS = libwine.a ntdll.a

TARGET_GCCLIBS = uuid gdi32 rpcrt4

TARGET_OBJECTS = \
	antimoniker.o \
	bindctx.o \
	clipboard.o \
	compobj.o \
	compositemoniker.o \
	datacache.o \
	defaulthandler.o \
	errorinfo.o \
	filemoniker.o \
	ftmarshal.o \
	git.o \
	hglobalstream.o \
	ifs.o \
	itemmoniker.o \
	marshal.o \
	memlockbytes.o \
	moniker.o \
	ole2.o \
	ole2stubs.o \
	ole2impl.o \
	ole32_main.o \
	oleobj.o \
	oleproxy.o \
	regsvr.o \
	rpc.o \
	stg_bigblockfile.o \
	stg_stream.o \
	storage32.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
