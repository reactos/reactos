# Copyright (c) 2001-2004 IBM, Inc. and others
# conversion sample code


# Usage:
#  - configure and build ICU [see the docs] .. use "--prefix=" something
#     (I  used --prefix=/home/srl/III )
# 
#  - do 'make install' of icu 
#
#  - set the variable ICU_PREFIX to point at $(prefix)
#    (will look for $(prefix)/lib/icu/Makefile.inc )
# 
#  - set the variable ICU_LEGACY  (read the README)
#   ( Will look for ICU_LEGACY/include/unicode/ucol.h )
#  
#  - do 'make' in this directory

#ICU_PREFIX=/home/weiv/build/current
#ICU_LEGACY = /home/weiv/build/icu-1-8-1

LEGACY_INCLUDE = $(ICU_LEGACY)/include

ICU_INC=$(ICU_PREFIX)/lib/icu/Makefile.inc
ICUPATH=

-include $(ICU_INC)

# Name of your target
TARGET=legacy

# All object files (C or C++)
OBJECTS=legacy.o newcol.o oldcol.o

CLEANFILES=*~ $(TARGET).out

DEPS=$(OBJECTS:.o=.d)

# turn on super warnings
#CPPFLAGS += -Wall

all: $(TARGET) $(ICU_INC)

.PHONY: all clean distclean check report

oldcol.o : oldcol.cpp $(LEGACY_INCLUDE)/unicode/ucol.h
	$(CXX) -I$(LEGACY_INCLUDE) $(DEFS) $(CPPFLAGS) $(CXXFLAGS) -c oldcol.cpp

distclean clean:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)
	-$(RMV) $(OBJECTS) $(TARGET)
	-$(RMV) $(DEPS)

# Can change this to LINK.c if it is a C only program
# Can add more libraries here. 
$(TARGET): $(OBJECTS)
	$(LINK.cc) -o $@ $^ $(ICULIBS) $(ICU_PREFIX)/lib/libicuuc.so.18 $(ICU_PREFIX)/lib/libicui18n.so.18 

# Make check: simply runs the sample, logged to a file
check: $(TARGET)
	$(INVOKE) ./$(TARGET) | tee $(TARGET).out

# Make report: creates a 'report file' with both source and sample run
report: $(TARGET).report

$(TARGET).report: check $(TARGET).cpp
	more $(TARGET).cpp $(TARGET).out > $@

$(ICU_INC):
	@echo ICU_PREFIX variable is not set correctly
	@echo "Please read the directions at the top of this file (Makefile)"
	@echo "And the README"
	@echo "Can't open $(ICU_INC)"
	@false

$(LEGACY_INCLUDE)/unicode/ucol.h:
	@echo ICU_LEGACY variable is not set correctly.
	@echo "Please read the directions at the top of this file (Makefile)"
	@echo "And the README"
	@echo "Can't open $@"
	@false

ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
