# $Id: makefile,v 1.85 2003/06/07 11:33:13 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = dynlink

TARGET_NAME = ntdll

TARGET_CFLAGS = -g -D__NTDLL__

TARGET_ASFLAGS = -I $(PATH_TO_TOP)/include

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles

TARGET_SDKLIBS = string.a rosrtl.a

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x77f60000

TARGET_PATH = def

TARGET_ENTRY = _LdrInitializeThunk@16

CSR_OBJECTS = csr/lpc.o csr/capture.o csr/probe.o csr/thread.o

DBG_OBJECTS = dbg/brkpoint.o dbg/debug.o dbg/print.o #dbg/winedbg.o

RTL_I386_OBJECTS = \
	rtl/i386/exception.o \
	rtl/i386/except.o \
	rtl/i386/ftol.o

RTL_OBJECTS = rtl/critical.o rtl/error.o rtl/heap.o rtl/largeint.o \
              rtl/math.o rtl/mem.o rtl/nls.o rtl/process.o rtl/sd.o \
              rtl/thread.o rtl/unicode.o rtl/env.o rtl/path.o rtl/ppb.o \
              rtl/bitmap.o rtl/time.o rtl/acl.o rtl/sid.o rtl/image.o \
              rtl/access.o rtl/apc.o rtl/callback.o rtl/luid.o rtl/misc.o \
              rtl/registry.o rtl/exception.o rtl/intrlck.o rtl/resource.o \
              rtl/handle.o rtl/atom.o rtl/message.o rtl/timezone.o \
      	      rtl/propvar.o rtl/security.o rtl/dos8dot3.o rtl/compress.o \
              rtl/encode.o rtl/random.o

STDIO_OBJECTS = stdio/sprintf.o stdio/swprintf.o

STDLIB_OBJECTS = stdlib/abs.o stdlib/atoi.o stdlib/atoi64.o stdlib/atol.o \
                 stdlib/itoa.o stdlib/itow.o stdlib/labs.o stdlib/splitp.o \
                 stdlib/strtol.o stdlib/strtoul.o stdlib/wcstol.o \
                 stdlib/wcstoul.o stdlib/wtoi.o stdlib/wtoi64.o stdlib/wtol.o \
                 stdlib/mbstowcs.o stdlib/wcstombs.o stdlib/qsort.o

STRING_OBJECTS = string/ctype.o  \
                 string/memicmp.o\
                 string/strcspn.o \
                 string/stricmp.o \
                 string/strlwr.o  \
                 string/strnicmp.o \
                 string/strpbrk.o  string/strspn.o \
                 string/strstr.o string/strupr.o string/wstring.o

ARCH_OBJECTS = \
	$(RTL_I386_OBJECTS)

TARGET_OBJECTS = \
  napi.o \
  ldr/entry.o \
  ldr/startup.o \
  $(ARCH_OBJECTS) \
  $(DBG_OBJECTS) \
  $(RTL_OBJECTS) \
  $(STDIO_OBJECTS) \
  $(STDLIB_OBJECTS) \
  $(STRING_OBJECTS) \
  stubs/stubs.o \
  ldr/res.o \
  ldr/utils.o \
  $(CSR_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)
DEP_EXCLUDE_FILTER = napi.%

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: csr/TAGS dbg/TAGS ldr/TAGS main/TAGS rtl/TAGS stdio/TAGS stdlib/TAGS string/TAGS stubs/TAGS
	etags -i csr/TAGS -i dbg/TAGS -i ldr/TAGS -i main/TAGS -i rtl/TAGS -i stdio/TAGS -i stdlib/TAGS -i string/TAGS -i stubs/TAGS
