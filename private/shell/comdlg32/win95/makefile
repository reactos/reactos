##########################################################################
#
#       Shell DLL Makefile
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


ROOT=..\..\..
SRCDIR=..\..
RES_DIR=..\..
NAME=comdlg32

BUILDDLL=TRUE
DLLBASE=PREFBASE
INFERPCH=TRUE

!ifdef VERDIR

ROOT=..\$(ROOT)
WIN32=TRUE

!else #VERDIR

VERSIONLIST = debug retail

!endif #VERDIR

CLEANLIST = *.rlb *.rxp *.pdb

!ifdef LEGO
LEGOCFLAGS= -Zi /Gfy /GX-
!endif

!ifndef ILINK
!if "$(VERDIR)" == "retail"
L32FLAGS=$(L32FLAGS) /ORDER:@$(SRCDIR)\comdlg32.ord
!endif
!endif


CFLAGS=$(CFLAGS) -Gy
AFLAGS=$(AFLAGS) -Gz -Zp4 -DSTD_CALL -DM4
RCFLAGS=$(RCFLAGS) -DWIN32

#-------------------------------------------------------------------------
# WIN32 object files and libraries
#-------------------------------------------------------------------------

PCHOBJ0 = color.obj color2.obj data.obj dlgs.obj fileopen.obj find.obj
PCHOBJ1 = font.obj init.obj parse.obj prnsetup.obj
MISCOBJ0 = filenew.obj

LIB0= kernel32.lib user32.lib gdi32.lib advapi32.lib shlwapi.lib shell32.lib comctl32.lib
!if "$(VERDIR)" == "debug"
LIB1= libcmt.lib
!endif

#-------------------------------------------------------------------------

!include $(ROOT)\shell\common.mk

!ifdef VERDIR

THKASM     = $(ROOT)\shell\thunk\$(VERDIR)\dlgthk.asm
dlgthk32.obj : $(THKASM)
        set ML=$(AFLAGS)
        $(ASM) -Fo$@ $(THKASM)

# To catch all the dependencies, use ours:
comdlg32.res : $(SRCDIR)\comdlg32.rc $(SRCDIR)\isz.h $(SRCDIR)\cdids.h $(SRCDIR)\sz.src
        $(RC) $(RCFLAGS) -fo$*.res -r $(SRCDIR)\comdlg32.rc

#
# Rule to generate source file dependencies
#

$(SRCDIR)\depend.mk::
!if "$(BUILD)" == "depend"
        echo # > $@
        echo # Warning: This file is generated automatically. >> $@
        echo # >> $@
        echo. >> $@
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< | sed "s/pch.obj/pch.obj .\\pchcpp.obj .\\comdlg32.pch .\\pchcpp.pch/" >> $@
-D..\..
pch.c
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..\..
-nshellprv.h ..\..\*.c ..\..\*.asm
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..\..
-npchcpp.h ..\..\*.cpp
<<
!if exist($(SRCDIR)\$(NAME).rc)
        $(INCLUDES) $(INCLUDES_SWITCHES) -C=acl -C=dlg -C=mnu -C=rc -C=stb -C=ver -sres @<< >> $@
$(NAME).rc
<<
!endif
!else
        @echo Use "nmake BUILD=depend" to build dependencies.
!endif


!endif #VERDIR

!endif # NTMAKEENV

