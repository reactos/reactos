##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991-96
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else

RES_DIR         = ..\..
PRIVINC         = rnaui
ROOT            = ..\..\..\..
SRCDIR          = ..\..
DLLBASE         = PREFBASE

CCSHELL_DIR     = $(ROOT)\shell

!ifdef VERDIR
ROOT            = ..\$(ROOT)
WIN32           = TRUE
CLEANLIST       = *.pdb *.rxp *.rlb
!else
VERSIONLIST     =debug retail
!endif

!ifdef LEGO
LEGOCFLAGS      = -Zi /Gfy /GX-
!endif

NAME            = rnaui
RESNAME         = rnaui.res
DEFNAME         = rnaui.def
RCNAME          = rnaui.rc
RCVNAME         = rnaui.rcv
RCFLAGS         = $(RCFLAGS) -DWIN32
CFLAGS          = $(CFLAGS) -Gy

DLLENTRY        = LibMain

!ifdef  WINDOWS_ME
CFLAGS          = $(CFLAGS) -DWINDOWS_ME -D$(WINDOWS_ME)
!endif

#-------------------------------------------------------------------------
# WIN32 object files and libraries
#-------------------------------------------------------------------------

CPPOBJS = sfvlist.obj

CODESEG0=_TEXT
PCHOBJ0 =contain.obj    subobj.obj      connent.obj     \
         conutil.obj    utils.obj       confirm.obj     \
         cstrings.obj   err.obj         mem.obj         \
         scripter.obj   mlink.obj       profile.obj     \
         rnahelp.obj    dataobj.obj     setting.obj

CODESEG1=_INIT
PCHOBJ1 =rnaui.obj

CODESEG2=_SETUP
PCHOBJ2 =rnawiz.obj


LIB0 =  kernel32.lib user32.lib gdi32.lib advapi32.lib \
        comdlg32.lib comctl32.lib
LIB1 =  shell32.lib shguidp.lib shlwapi.lib ole32.lib
LIB2 =  $(SRCDIR)\lib\rasapi32.lib
LIB3 =  libc.lib # we're really close to not needing this at all

#-------------------------------------------------------------------------

BUILDDLL        = TRUE

!include $(ROOT)\shell\common.mk

INCLUDE=$(INCLUDE);$(SRCDIR)\inc

!ifdef VERDIR

$(RESNAME):    $(SRCDIR)\rcids.h

AFLAGS32        = -Gz -Zp4 -DSTD_CALL -DM4

init32.obj: ..\$(@B).asm
    set OLDML=%ML%
    set ML=$(AFLAGS) $(AFLAGS32)
    set INCLUDE=$(THUNKDIR);$(INCLUDE)
    mlx -Fo$@ ..\$(@B).asm
    set ML=%OLDML%

#
# Rule to generate source file dependencies
#

$(SRCDIR)\depend.mk::
!if "$(BUILD)" == "depend"
        echo # > $@
        echo # Warning: This file is generated automatically. >> $@
        echo # >> $@
        echo. >> $@
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< | sed "s/pch.obj/pch.obj .\\pchcpps.obj .\\shellprv.pch .\\pchcpps.pch/" >> $@
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

!endif # !VERDIR

!endif # NTMAKEENV

