# $Id: makefile,v 1.1 2004/01/10 20:38:17 arty Exp $
# Kernel JavaScript
# 
# You can use this for various things but the most obvious is as a powerful
# interactive debugger.
#
# Other modules might use it as well to prototype parts of the kernel, and
# to implement policy.  Since kjs can read the registry, it is possible to
# load libraries from there.  Also, I'm adding an easy interface that allows
# a module to introduce its own kjs methods.
#

PATH_TO_TOP = ../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = library

TARGET_NAME = kjs

TARGET_CFLAGS = -g -D__NTKJS__ -Werror -Wall -I. -Isrc -Iinclude

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_ASFLAGS = -I $(PATH_TO_TOP)/include

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles -nostdlib \

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x79f60000

TARGET_PATH = 

#
# TARGET_ENTRY = 0x0
#

ARCH_OBJECTS = ksrc/setjmp.o ksrc/longjmp.o
MOD_OBJECTS  = ksrc/alloc.o \
	       ksrc/bc.o \
	       ksrc/b_core.o \
	       ksrc/b_file.o \
	       ksrc/b_func.o \
	       ksrc/b_regexp.o \
	       ksrc/b_system.o \
	       ksrc/compat.o \
	       ksrc/debug.o \
	       ksrc/iostream.o \
	       ksrc/js.o \
	       ksrc/kjs.o \
	       ksrc/mrgsort.o \
	       ksrc/object.o \
	       ksrc/regex.o \
	       ksrc/vm.o \
	       ksrc/vmjumps.o \
	       ksrc/vmswitch.o \
	       ksrc/vmswt0.o
ORIG_OBJECTS = src/b_array.o \
	       src/b_bool.o \
	       src/b_object.o \
	       src/b_number.o \
	       src/b_string.o \
	       src/b_vm.o \
	       src/compiler.o \
	       src/crc32.o \
	       src/dl_dummy.o \
	       src/heap.o \
	       src/utils.o
TARGET_OBJECTS = $(ORIG_OBJECTS) $(MOD_OBJECTS) $(ARCH_OBJECTS) \
		../../dk/w32/lib/ntdll.a
DEP_OBJECTS = $(TARGET_OBJECTS)

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
