
PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = rtl

include $(PATH_TO_TOP)/config

TARGET_CFLAGS = -Wall -Werror -ffreestanding

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = \
	acl.o \
	compress.o \
	dos8dot3.o \
	env.o \
	error.o \
	heap.o \
	largeint.o \
	luid.o \
	mem.o \
	nls.o \
	random.o \
	sd.o \
	security.o \
	sid.o \
	time.o \
	timezone.o \
	unicode.o \
	version.o \
	encode.o \
	image.o \
	splaytree.o \
        exception.o \
        i386/exception.o \
        i386/except.o

#	atom
#	registry
 

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

DEP_OBJECTS := $(TARGET_OBJECTS)

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/tools/depend.mk

# EOF
