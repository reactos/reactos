# $Id: Makefile 12745 2005-01-03 02:37:10Z sedwards $

PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -D__USE_W32API -D_WIN32_IE=0x0600 \
	-D_WIN32_WINNT=0x0501 -D__REACTOS__

TARGET_NAME = advapi32_test

TARGET_SDKLIBS = advapi32.a ntdll.a wine.a

TARGET_OBJECTS = \
	testlist.o \
	crypt.o \
	crypt_lmhash.o \
	crypt_md4.o \
	crypt_md5.o \
	crypt_sha.o \
	registry.o \
	security.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF