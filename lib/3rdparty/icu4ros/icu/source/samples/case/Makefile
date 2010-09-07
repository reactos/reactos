# Copyright (c) 2003 IBM, Inc. and others
# sample code makefile

# Usage:
#  - configure, build, install ICU (make install)
#  - make sure "icu-config" (in the ICU installed bin directory) is on
#     the path
#  - do 'make' in this directory

#### definitions
# Name of your target
TARGET=case

# All object files (C or C++)
OBJECTS=case.o ucase.o

#### rules
# Load in standard makefile definitions
include ../defs.mk

LDFLAGS += $(LDFLAGS_USTDIO)

# the actual rules (this is a simple sample)
include ../rules.mk
