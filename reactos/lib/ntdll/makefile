# $Id: makefile,v 1.72 2002/08/10 21:57:08 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ntdll

TARGET_CFLAGS = -g -D__NTDLL__

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x77f60000

TARGET_PATH = def

TARGET_ENTRY = _LdrInitializeThunk@16

CSR_OBJECTS = csr/lpc.o csr/capture.o csr/probe.o csr/thread.o

DBG_OBJECTS = dbg/brkpoint.o dbg/debug.o dbg/print.o dbg/winedbg.o

RTL_OBJECTS = rtl/critical.o rtl/error.o rtl/heap.o rtl/largeint.o \
              rtl/math.o rtl/mem.o rtl/nls.o rtl/process.o rtl/sd.o \
              rtl/thread.o rtl/unicode.o rtl/env.o rtl/path.o rtl/ppb.o \
              rtl/bitmap.o rtl/time.o rtl/acl.o rtl/sid.o rtl/image.o \
              rtl/access.o rtl/apc.o rtl/callback.o rtl/luid.o rtl/misc.o \
              rtl/registry.o rtl/exception.o rtl/intrlck.o rtl/resource.o \
              rtl/handle.o rtl/atom.o rtl/message.o rtl/timezone.o \
      	      rtl/propvar.o rtl/security.o rtl/dos8dot3.o rtl/compress.o

STDIO_OBJECTS = stdio/sprintf.o stdio/swprintf.o

STDLIB_OBJECTS = stdlib/abs.o stdlib/atoi.o stdlib/atoi64.o stdlib/atol.o \
                 stdlib/itoa.o stdlib/itow.o stdlib/labs.o stdlib/splitp.o \
                 stdlib/strtol.o stdlib/strtoul.o stdlib/wcstol.o \
                 stdlib/wcstoul.o stdlib/wtoi.o stdlib/wtoi64.o stdlib/wtol.o \
                 stdlib/mbstowcs.o stdlib/wcstombs.o stdlib/qsort.o

STRING_OBJECTS = string/ctype.o string/memccpy.o string/memchr.o \
                 string/memcmp.o string/memcpy.o string/memicmp.o\
                 string/memmove.o string/memset.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcspn.o \
                 string/strcpy.o string/stricmp.o string/strlen.o \
                 string/strlwr.o string/strncat.o string/strncmp.o \
                 string/strncpy.o string/strnicmp.o string/strnlen.o \
                 string/strpbrk.o string/strrchr.o string/strspn.o \
                 string/strstr.o string/strupr.o string/wstring.o

TARGET_OBJECTS = \
  napi.o \
  ldr/startup.o \
  $(DBG_OBJECTS) \
  $(RTL_OBJECTS) \
  $(STDIO_OBJECTS) \
  $(STDLIB_OBJECTS) \
  $(STRING_OBJECTS) \
  stubs/stubs.o \
  ldr/utils.o \
  $(CSR_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: csr/TAGS dbg/TAGS ldr/TAGS main/TAGS rtl/TAGS stdio/TAGS stdlib/TAGS string/TAGS stubs/TAGS
	etags -i csr/TAGS -i dbg/TAGS -i ldr/TAGS -i main/TAGS -i rtl/TAGS -i stdio/TAGS -i stdlib/TAGS -i string/TAGS -i stubs/TAGS
