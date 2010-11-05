# Copyright (c) 2000-2002 IBM, Inc. and others
# sample code makefile

# Usage:
#  - configure, build, install ICU (make install)
#  - make sure "icu-config" (in the ICU installed bin directory) is on
#     the path
#  - do 'make' in this directory

# Name of your target
TARGET=convsamp

# All object files (C or C++)
OBJECTS=convsamp.o flagcb.o

# Load in standard makefile definitions
include ../defs.mk

# extra files to clean up
CLEANFILES += *.out data40.utf16 data41.utf16

# the actual rules (this is a simple sample)
include ../rules.mk
