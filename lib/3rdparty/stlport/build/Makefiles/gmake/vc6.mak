# Time-stamp: <07/03/08 21:41:52 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

#INCLUDES = -I$(SRCROOT)/include
#INCLUDES :=

CXX := cl.exe
CC := cl.exe
LINK := link.exe
RC := rc.exe

DEFS ?=
OPT ?=

# OUTPUT_OPTION = /Fo$@
release-shared:	OUTPUT_OPTION = /Fo$@
release-static:	OUTPUT_OPTION = /Fo$@
dbg-shared :	OUTPUT_OPTION = /Fo$@ /Fd"${OUTPUT_DIR_DBG}"
stldbg-shared :	OUTPUT_OPTION = /Fo$@ /Fd"${OUTPUT_DIR_STLDBG}"
dbg-static :	OUTPUT_OPTION = /Fo$@ /Fd"${OUTPUT_DIR_A_DBG}"
stldbg-static :	OUTPUT_OPTION = /Fo$@ /Fd"${OUTPUT_DIR_A_STLDBG}"
LINK_OUTPUT_OPTION = /OUT:$@
RC_OUTPUT_OPTION = /fo $@
DEFS += /D "WIN32" /D "_WINDOWS"
CPPFLAGS = $(DEFS) $(INCLUDES)

CFLAGS = /nologo /TC /W3 /GR /GX /Zm800 $(OPT)
CXXFLAGS = /nologo /TP /W3 /GR /GX /Zm800 $(OPT)
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) /c
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) /c
LINK.cc = $(LINK) /nologo $(LDFLAGS) $(TARGET_ARCH)
COMPILE.rc = $(RC) $(RCFLAGS)

CDEPFLAGS = /FD /E
CCDEPFLAGS = /FD /E

# STLport DEBUG mode specific defines
stldbg-static :	    DEFS += /D_DEBUG /D "_STLP_DEBUG"
stldbg-shared :     DEFS += /D_DEBUG /D "_STLP_DEBUG"
stldbg-static-dep : DEFS += /D_DEBUG /D "_STLP_DEBUG"
stldbg-shared-dep : DEFS += /D_DEBUG /D "_STLP_DEBUG"

dbg-static :	 DEFS += /D_DEBUG
dbg-shared :     DEFS += /D_DEBUG
dbg-static-dep : DEFS += /D_DEBUG
dbg-shared-dep : DEFS += /D_DEBUG

release-static :	 DEFS += /DNDEBUG
release-shared :     DEFS += /DNDEBUG
release-static-dep : DEFS += /DNDEBUG
release-shared-dep : DEFS += /DNDEBUG

# optimization and debug compiler flags
release-static : OPT += /O2 /Og
release-shared : OPT += /O2 /Og

dbg-static : OPT += /Zi
dbg-shared : OPT += /Zi
#dbg-static-dep : OPT += -g
#dbg-shared-dep : OPT += -g

stldbg-static : OPT += /Zi
stldbg-shared : OPT += /Zi
#stldbg-static-dep : OPT += -g
#stldbg-shared-dep : OPT += -g

# dependency output parser (dependencies collector)

# oh, there VC is no mode has no options to print dependencies
# in more-or-less acceptable format. I use VC as preprocessor
# and see first line (here VC print file name).

# bug here: if no dependencies:
# ---------------------------------
#    int main() { return 0; }
# ---------------------------------
# this sed script produce wrong output
# ---------------------------------
#   obj/vc6/shared/xx.o obj/vc6/shared/xx.d : obj/vc6/shared/xx.cpp \
# ---------------------------------
# (wrong backslash at eol)


DP_OUTPUT_DIR = | grep "^\#line 1 " | (echo -e 's|\([a-zA-Z]\):|/cygdrive/\1|g\nt next\n: next\n1s|^\#line 1 \(.*\)|$(OUTPUT_DIR)/$*.o $@ : $< \\\\|\nt\n$$s|^\#line 1 "\(.*\)"|\1|g\nt space\ns|^\#line 1 "\(.*\)"|\1\\\\|g\nt space\nd\n: space\ns| |\\\\ |g\ns|^|  |\ns|\\\\\\\\|/|g\n' > $(OUTPUT_DIR)/tmp.sed; sed -f $(OUTPUT_DIR)/tmp.sed; rm -f $(OUTPUT_DIR)/tmp.sed ) > $@; \
                  [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_DBG = | grep "^\#line 1 " | (echo -e 's|\([a-zA-Z]\):|/cygdrive/\1|g\nt next\n: next\n1s|^\#line 1 \(.*\)|$(OUTPUT_DIR_DBG)/$*.o $@ : $< \\\\|\nt\n$$s|^\#line 1 "\(.*\)"|\1|g\nt space\ns|^\#line 1 "\(.*\)"|\1\\\\|g\nt space\nd\n: space\ns| |\\\\ |g\ns|^|  |\ns|\\\\\\\\|/|g\n' > $(OUTPUT_DIR_DBG)/tmp.sed; sed -f $(OUTPUT_DIR_DBG)/tmp.sed; rm -f $(OUTPUT_DIR_DBG)/tmp.sed ) > $@; \
                  [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_STLDBG = | grep "^\#line 1 " | (echo -e 's|\([a-zA-Z]\):|/cygdrive/\1|g\nt next\n: next\n1s|^\#line 1 \(.*\)|$(OUTPUT_DIR_STLDBG)/$*.o $@ : $< \\\\|\nt\n$$s|^\#line 1 "\(.*\)"|\1|g\nt space\ns|^\#line 1 "\(.*\)"|\1\\\\|g\nt space\nd\n: space\ns| |\\\\ |g\ns|^|  |\ns|\\\\\\\\|/|g\n' > $(OUTPUT_DIR_STLDBG)/tmp.sed; sed -f $(OUTPUT_DIR_STLDBG)/tmp.sed; rm -f $(OUTPUT_DIR_STLDBG)/tmp.sed ) > $@; \
                  [ -s $@ ] || rm -f $@
