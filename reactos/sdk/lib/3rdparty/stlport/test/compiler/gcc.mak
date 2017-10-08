# -*- Makefile -*- Time-stamp: <04/03/14 23:50:57 ptr>

SRCROOT := ../../build
COMPILER_NAME := gcc

ALL_TAGS := compile-only
STLPORT_DIR := ../..
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

compile-only:	$(OUTPUT_DIRS) $(OBJ)

