# $Id: makefile,v 1.100 2004/05/13 21:01:14 navaraf Exp $

PATH_TO_TOP = ../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = dynlink

TARGET_NAME = ntdll

TARGET_CFLAGS = -g -D__NTDLL__ -Werror -Wall

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_ASFLAGS = -I $(PATH_TO_TOP)/include

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles -nostdlib

TARGET_SDKLIBS = string.a rosrtl.a

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x77f80000

TARGET_PATH = def

TARGET_ENTRY = 0x0

CSR_OBJECTS = \
	csr/capture.o \
	csr/lpc.o \
	csr/probe.o \
	csr/thread.o

DBG_OBJECTS = \
	dbg/brkpoint.o \
	dbg/debug.o \
	dbg/print.o
#	dbg/winedbg.o

LDR_OBJECTS = \
	ldr/entry.o \
	ldr/res.o \
	ldr/startup.o \
	ldr/utils.o

RTL_I386_OBJECTS = \
	rtl/i386/alldiv.o \
	rtl/i386/allmul.o \
	rtl/i386/allrem.o \
	rtl/i386/allshl.o \
	rtl/i386/allshr.o \
	rtl/i386/aulldiv.o \
	rtl/i386/aullrem.o \
	rtl/i386/aullshr.o \
	rtl/i386/chkstk.o \
	rtl/i386/exception.o \
	rtl/i386/except.o \
	rtl/i386/float.o \
	rtl/i386/ftol.o

RTL_OBJECTS = \
	rtl/access.o \
	rtl/acl.o \
	rtl/apc.o \
	rtl/atom.o \
	rtl/bitmap.o \
	rtl/callback.o \
	rtl/compress.o \
	rtl/crc32.o \
	rtl/critical.o \
	rtl/dos8dot3.o \
	rtl/error.o \
	rtl/encode.o \
	rtl/env.o \
	rtl/exception.o \
	rtl/handle.o \
	rtl/heap.o \
	rtl/image.o \
	rtl/intrlck.o \
	rtl/largeint.o \
	rtl/luid.o \
	rtl/math.o \
	rtl/mem.o \
	rtl/message.o \
	rtl/misc.o \
	rtl/nls.o \
	rtl/path.o \
	rtl/ppb.o \
	rtl/process.o \
	rtl/propvar.o \
	rtl/random.o \
	rtl/rangelist.o \
	rtl/registry.o \
	rtl/resource.o \
	rtl/sd.o \
	rtl/security.o \
	rtl/sid.o \
	rtl/splaytree.o \
	rtl/teb.o \
	rtl/thread.o \
	rtl/time.o \
	rtl/timezone.o \
	rtl/unicode.o \
	rtl/version.o

STDIO_OBJECTS = \
	stdio/sprintf.o \
	stdio/sscanf.o \
	stdio/swprintf.o

STDLIB_OBJECTS = \
	stdlib/abs.o \
	stdlib/atoi.o \
	stdlib/atoi64.o \
	stdlib/atol.o \
	stdlib/bsearch.o \
	stdlib/itoa.o \
	stdlib/itow.o \
	stdlib/labs.o \
	stdlib/lfind.o \
	stdlib/mbstowcs.o \
	stdlib/splitp.o \
	stdlib/strtol.o \
	stdlib/strtoul.o \
	stdlib/wcstol.o \
	stdlib/wcstombs.o \
	stdlib/wcstoul.o \
	stdlib/wtoi.o \
	stdlib/wtoi64.o \
	stdlib/wtol.o

STRING_OBJECTS = \
	string/ctype.o \
	string/memicmp.o \
	string/stricmp.o \
	string/strlwr.o \
	string/strnicmp.o \
	string/strpbrk.o \
	string/strstr.o \
	string/strupr.o \
	string/wstring.o

ARCH_OBJECTS = \
	$(RTL_I386_OBJECTS)

TARGET_OBJECTS = \
	napi.o \
	$(ARCH_OBJECTS) \
	$(CSR_OBJECTS) \
	$(DBG_OBJECTS) \
	$(LDR_OBJECTS) \
	$(RTL_OBJECTS) \
	$(STDIO_OBJECTS) \
	$(STDLIB_OBJECTS) \
	$(STRING_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)
DEP_EXCLUDE_FILTER = napi.%

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: csr/TAGS dbg/TAGS ldr/TAGS main/TAGS rtl/TAGS stdio/TAGS stdlib/TAGS string/TAGS stubs/TAGS
	etags -i csr/TAGS -i dbg/TAGS -i ldr/TAGS -i main/TAGS -i rtl/TAGS -i stdio/TAGS -i stdlib/TAGS -i string/TAGS -i stubs/TAGS


docu:
	doxygen Doxyfile

.PHONY: docu


# EOF
