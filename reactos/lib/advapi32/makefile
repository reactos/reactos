# $Id: makefile,v 1.41 2004/06/17 09:07:12 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = advapi32

TARGET_CFLAGS = -Wall -Werror -fno-builtin -D__USE_W32API
# TARGET_CFLAGS += -DUNICODE

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_BASE = $(TARGET_BASE_LIB_ADVAPI32)

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
	sec/sid.o

SERVICE_OBJECTS = \
	service/eventlog.o \
	service/scm.o \
	service/sctrl.o \
	service/undoc.o

TOKEN_OBJECTS = \
	token/privilege.o \
	token/token.o

TARGET_OBJECTS = \
	$(MISC_OBJECTS) \
	$(REGISTRY_OBJECTS) \
	$(SECURITY_OBJECTS) \
	$(SERVICE_OBJECTS) \
	$(TOKEN_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
