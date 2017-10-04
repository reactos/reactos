# -*- Makefile -*- Time-stamp: <03/07/09 18:08:47 ptr>

SRCROOT := ../../../build
COMPILER_NAME := gcc

# WITHOUT_STLPORT = 1
# STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

#CXXFLAGS += -fuse-cxa-atexit
LDFLAGS += -Wl,-rpath=${STLPORT_LIB_DIR}
