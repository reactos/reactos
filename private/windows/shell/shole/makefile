##########################################################################
#
#       Copyright (C) Microsoft Corporation 1994
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

#
# Build using NMAKE.EXE
#

NAME=shscrap
ROOT=..\..\..\..
RES_DIR=..
WIN32=TRUE
OLENOT=TRUE
PRIVINC=shole
DEFNAME=shole.def

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

RESNAME=shole.res
RCNAME=shole.rc
RCVNAME=shole.rcv
#DEFNAME=shole.def
FORCE_CPP=-Tp

DLLENTRY = LibMain

PCHOBJ0=shole.obj debug.obj scdata.obj sccls.obj sccreate.obj
LIB0 = kernel32.lib user32.lib gdi32.lib mpr.lib ole32.lib uuid.lib \
       shell32.lib comdlg32.lib advapi32.lib crtdll.lib comctl32.lib libcmt.lib

BUILDDLL=TRUE

!include $(ROOT)\win\core\shell\common.mk

!ifdef VERDIR

$(PRIVINC).pch: $(ROOT)\dev\inc\shlobj.h

shole.res:

!endif

!endif # NTMAKEENV
