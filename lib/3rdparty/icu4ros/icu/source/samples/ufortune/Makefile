# Copyright (c) 2001-2003 IBM, Inc. and others

#
# File
#    icu/source/samples/ufortune/Makefile
#
# Usage:
#  - configure and build ICU [see the docs] .. use "--prefix=" something
# 
#  - do 'make install' of icu 
#
#  - make sure the script 'icu-config' is executable in your PATH
#  
#  - do 'gmake' in this directory
#  - do 'gmake check' to run the sample.


# Load ICU information
include ../defs.mk

include fortunedefs.mk

# Name of your target
TARGET=ufortune

# All object files (C or C++)
OBJECTS=ufortune.o

# dir containing resources
RESDIR=resources

# hook variables to tell rules.mk we want subdirectory processing
ALL_SUBDIR=   all-$(RESDIR)
CLEAN_SUBDIR= clean-$(RESDIR)

# Need: ustdio, and to link with ufortune
XTRALIBS=$(RESLDFLAGS)
LDFLAGS += $(LDFLAGS_USTDIO)
CPPFLAGS += $(RESCPPFLAGS)

### Include standard rules
include ../rules.mk

# subdirectory processing
all-$(RESDIR):
	$(MAKE) -C $(RESDIR)

clean-$(RESDIR):
	-$(MAKE) -C $(RESDIR) clean


install-$(RESDIR):
	$(MAKE) -C $(RESDIR) install

install: install-$(RESDIR)