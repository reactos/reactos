##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991-96
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

clean :

!else  # NTMAKEENV


RES_DIR         = ..
PRIVINC         = priv
ROOT            = ..\..
CCSHELL_DIR     = $(ROOT)\shell

# Define VCDEBUG to build a version that can be debugged under VC++

!if (defined(VCDEBUG) && ("$(VERDIR)" == "retail")) || !defined(VCDEBUG)
DLLBASE         = PREFBASE
!endif

!ifdef VERDIR

ROOT            = ..\$(ROOT)
WIN32           = TRUE
CLEANLIST       = *.pdb *.rlb *.rxp

!else

VERSIONLIST     = debug retail
!if "$(BUILD)" == "clean"
VERSIONLIST     = $(VERSIONLIST) debug retail
!endif

!endif  # VERDIR

BUILDDLL        = TRUE
NAME            = shlwapi
RESNAME         = shlwapi.res
RCNAME          = shlwapi.rc
RCVNAME         = shlwapi.rcv
DLLENTRY        = DllMain

!if 0
!ifndef ILINK
L32FLAGS        = /ORDER:@..\..\shlwapi.ord
!endif
!endif

CFLAGS          = $(CFLAGS) -Gy
!ifdef  DBCS
CFLAGS          = $(CFLAGS) -DFE_IME
!endif

AFLAGS          = $(AFLAGS)

!ifdef  LIST
MAKELIST        = TRUE
!endif

PCHOBJ1         = dll.obj debug.obj strings.obj reg.obj url.obj
PCHOBJ0         = strtable.obj reginst.obj inf.obj infcmd.obj eproc.obj convert.obj

MISCOBJ0        = patha.obj pathw.obj

#
# As you may or may not know, the order of this lib list is significant
# in two ways.
# (1) If there are duplicates, we pick the one from the lib on left.
# (2) Rellocation tables for the right most libs are placed at the lower
#    address.
#
LIB0            = kernel32.lib gdi32.lib user32.lib advapi32.lib
!if defined(VERDIR) && "$(VERDIR)" == "debug"
LIB1            = $(CCSHELL_DIR)\lib\$(VERDIR)\stocklib.lib
!endif
!if "$(VERDIR)" == "debug"
LIB0            = $(LIB0) libcmt.lib
!endif

#========================================================================

!include $(ROOT)\shell\common.mk

!ifdef VERDIR

!if !exist($(CCSHELL_DIR)\lib\$(VERDIR)\stocklib.lib)
!message ****
!message **** Warning: You need to ssync -ur ccshell\lib
!message ****
!error
!endif

$(SRCDIR)\shlwapi.def: $(SRCDIR)\shlwapi.src
        cl -EP $? > $@

$(SRCDIR)\patha.c: $(SRCDIR)\path.c
        echo #undef UNICODE > $@
        type $? >> $@

$(SRCDIR)\pathw.c: $(SRCDIR)\path.c
        echo #define UNICODE > $@
        type $? >> $@

#
# Rule to generate source file dependencies
#

$(SRCDIR)\depend.mk::
!if "$(BUILD)" == "depend"
        echo # > $@
        echo # Warning: This file is generated automatically. >> $@
        echo # >> $@
        echo. >> $@
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< | sed "s/pch.obj/pch.obj .\\priv.pch/" >> $@
-D..\..
pch.c
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..\..
-npriv.h ..\..\*.c ..\..\*.asm
<<
!if exist($(SRCDIR)\$(NAME).rc)
        $(INCLUDES) $(INCLUDES_SWITCHES) -C=acl -C=dlg -C=mnu -C=rc -C=stb -C=ver -sres @<< >> $@
-D..\..
$(NAME).rc
<<
!endif
!else
        @echo Use "nmake BUILD=depend" to build dependencies.
!endif


!else

!if "$(BUILD)" != "clean"
all:    debug retail

!if "$(BUILD)" == "debug"
default:	debug
!endif

!if "$(BUILD)" == "retail"
default:	retail
!endif

!endif

.w.h:
        $(DEVROOT)\tools\binw\hsplit -o $*.h $(*)p.h $*.w


.pef.def:
        $(DEVROOT)\tools\binw\hsplit -o $*.def $(*)p.def $*.pef


!endif # !VERDIR
        
!endif # NTMAKEENV
