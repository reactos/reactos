# $Id: makefile,v 1.21 2001/11/22 02:37:06 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = advapi32

TARGET_SDKLIBS = ntdll.a kernel32.a

# TARGET_CFLAGS = -DUNICODE

TARGET_BASE = 0x77dc0000

MISC_OBJECTS=\
	misc/dllmain.o	\
	misc/shutdown.o	\
	misc/sysfunc.o 

REGISTRY_OBJECTS=\
	reg/reg.o

SECURITY_OBJECTS=\
	sec/ac.o	\
	sec/lsa.o	\
	sec/misc.o	\
	sec/sec.o	\
	sec/sid.o

SERVICE_OBJECTS=\
	service/scm.o	\
	service/sctrl.o	\
	service/undoc.o

TOKEN_OBJECTS=\
	token/privilege.o	\
	token/token.o

TARGET_OBJECTS = \
	$(MISC_OBJECTS)	\
	$(REGISTRY_OBJECTS) \
	$(SECURITY_OBJECTS) \
	$(SERVICE_OBJECTS) \
	$(TOKEN_OBJECTS)

TARGET_CLEAN = \
  misc/*.o \
  reg/*.o \
  sec/*.o \
  service/*.o \
  token/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
