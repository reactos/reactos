# $Id: Makefile,v 1.1 2003/05/27 18:56:15 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = string

include $(PATH_TO_TOP)/config

include makefile.$(ARCH)

TARGET_CFLAGS = -Wall -Werror

ifeq ($(DBG), 1)
  TARGET_CFLAGS += -g
else
  TARGET_CFLAGS += -fno-strict-aliasing -O6 
endif

TARGET_OBJECTS = \
 	memchr.o \
	memcmp.o \
	memcpy.o \
	memmove.o \
	memset.o \
	strcat.o \
	strchr.o \
	strcmp.o \
	strcpy.o \
	strlen.o \
	strncat.o \
	strncmp.o \
	strncpy.o \
	strnlen.o \
	strrchr.o \
	wcscat.o \
	wcschr.o \
	wcscmp.o \
	wcscpy.o \
	wcslen.o \
	wcsncat.o \
	wcsncmp.o \
	wcsncpy.o \
	wcsnlen.o \
	wcsrchr.o

TARGET_OBJECTS := $(filter-out $(EXCLUDE_FILTER), $(TARGET_OBJECTS)) $(OBJECTS_ARCH)

 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/tools/depend.mk
