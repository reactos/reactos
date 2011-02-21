# Copyright (c) 2000-2004 IBM, Inc. and others
# udata sample code
# Usage:
#  - configure, build, install ICU
#  - ensure that 'icu-config' is in the PATH (PREFIX/bin/icu-config) 
#  - if ICU is not built relative to this directory,
#      set the variable ICU_PATH to the 'icu' directory
#       (i.e.  /foo/icu  )
#  - do 'make' in this directory


# Include ICU standard definitions
include ../defs.mk

# look for the ICU_PATH variable, guess if not there
ICU_DEFAULT_PATH=../../..

ifeq ($(strip $(ICU_PATH)),)
  ICU_PATH=$(ICU_DEFAULT_PATH)
endif

# Name of your target
TARGET1=reader
TARGET2=writer

# All object files (C or C++)
OBJECTS1=reader.o
OBJECTS2=writer.o
OBJECTS=$(OBJECTS1) $(OBJECTS2)

CLEANFILES=*~ $(TARGET).out

all: $(TARGET1) $(TARGET2)

# The following lines are to make sure ICU_PATH is set properly.
writer.o: $(ICU_PATH)/source/tools/toolutil/uoptions.h

$(ICU_PATH)/source/tools/toolutil/uoptions.h:
	@echo
	@echo "*** Please read the directions at the top of this file (Makefile)"
	@echo "*** Can't open $@ - check ICU_PATH"
	@echo
	@false

# Only the writer needs these, actually.
CPPFLAGS += -I$(ICU_PATH)/source/tools/toolutil
LDFLAGS += -L$(ICU_PATH)/source/tools/toolutil $(shell icu-config --ldflags-toolutil)


.PHONY: all clean distclean check report

distclean clean:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)
	-$(RMV) $(OBJECTS) $(TARGET1) $(TARGET2)

# Can change this to LINK.c if it is a C only program
# Can add more libraries here. 
$(TARGET1): $(OBJECTS1)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET2): $(OBJECTS2)
	$(CC) -o $@ $^ $(LDFLAGS)

# Make check: simply runs the sample, logged to a file
check: $(TARGET1) $(TARGET2)
	$(INVOKE) ./$(TARGET2) | tee $(TARGET2).out
	$(INVOKE) ./$(TARGET1) | tee $(TARGET1).out


