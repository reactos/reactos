# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = advapi32

TARGET_PCH = advapi32.h

TARGET_CFLAGS = -Wall -Werror -fno-builtin \
                -D__USE_W32API \
                -D_WIN32_IE=0x0500 \
                -D_WIN32_WINNT=0x501 \
                -DWINVER=0x600 \

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_BASE = $(TARGET_BASE_LIB_ADVAPI32)

CRYPT_OBJECTS = \
	crypt/crypt_des.o \
	crypt/crypt_lmhash.o \
	crypt/crypt_md4.o \
	crypt/crypt_md5.o \
	crypt/crypt_sha.o \
	crypt/crypt.o

MISC_OBJECTS = \
	misc/dllmain.o \
	misc/hwprofiles.o \
	misc/logon.o \
	misc/shutdown.o \
	misc/sysfunc.o \

REGISTRY_OBJECTS = \
	reg/reg.o

SECURITY_OBJECTS = \
	sec/ac.o \
	sec/audit.o \
	sec/lsa.o \
	sec/misc.o \
	sec/sec.o \
	sec/sid.o \
	sec/trustee.o

SERVICE_OBJECTS = \
	service/eventlog.o \
	service/scm.o \
	service/sctrl.o \
	service/undoc.o

TOKEN_OBJECTS = \
	token/privilege.o \
	token/token.o

TARGET_OBJECTS = \
	$(CRYPT_OBJECTS) \
	$(MISC_OBJECTS) \
	$(REGISTRY_OBJECTS) \
	$(SECURITY_OBJECTS) \
	$(SERVICE_OBJECTS) \
	$(TOKEN_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
