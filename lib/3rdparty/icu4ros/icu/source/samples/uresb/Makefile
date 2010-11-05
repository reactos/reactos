# Copyright (c) 2000-2005 IBM, Inc. and others
# conversion sample code
# Usage:
#  - configure, build, install ICU
#  - ensure that 'icu-config' is in the PATH (PREFIX/bin/icu-config) 
#  - if ICU is not built relative to this directory,
#      set the variable ICU_PATH to the 'icu' directory
#       (i.e.  /foo/icu  )
#  - do 'make' in this directory

include ../defs.mk

ICU_DEFAULT_PATH=../../..

ifeq ($(strip $(ICU_PATH)),)
  ICU_PATH=$(ICU_DEFAULT_PATH)
endif

GENRBOPT = -s. -d.

# Name of your target
TARGET=uresb
PKG=$(TARGET)
RES_SRC=root.txt en.txt sr.txt
RESOURCES=$(RES_SRC:%.txt=%.res)

# All object files (C or C++)
OBJECTS=uresb.o

CLEANFILES=*~ $(TARGET).out

all: $(RESOURCES) $(TARGET) 

uresb.o:  $(ICU_PATH)/source/tools/toolutil/uoptions.h

$(ICU_PATH)/source/tools/toolutil/uoptions.h:
	@echo "Please read the directions at the top of this file (Makefile)"
	@echo "Can't open $@ - check ICU_PATH"
	@false

CPPFLAGS += -I$(ICU_PATH)/source/tools/toolutil
LDFLAGS += -L$(ICU_PATH)/source/tools/toolutil $(shell icu-config --ldflags-toolutil --ldflags-icuio)

.PHONY: all clean distclean check report

distclean clean:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)
	-$(RMV) $(OBJECTS) $(TARGET) $(RESOURCES)


## resources
%.res: %.txt
	@echo "generating $@"
	$(GENRB) $(GENRBOPT) $^

## Special for a special codepage
sr.res : sr.txt
	@echo "generating $@"
	$(GENRB) $(GENRBOPT) -e cp1251 $?

# Can change this to LINK.c if it is a C only program
# Can add more libraries here. 
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Make check: simply runs the sample, logged to a file
check: $(TARGET) $(RESOURCES)
	$(INVOKE) ./$(TARGET) en | tee $(TARGET).out

# Make report: creates a 'report file' with both source and sample run
report: $(TARGET).report

$(TARGET).report: check $(TARGET).cpp
	more $(TARGET).cpp $(TARGET).out > $@



