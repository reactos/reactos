#----------------------------------------------------------------------------
# Build the registr editor
#----------------------------------------------------------------------------

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

#
# Build using NMAKE.EXE
#

# Makefile : Build the registry editor
##########################################################################
#
#	Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1993-1994
#	All Rights Reserved.
#
##########################################################################

!ifndef VERDIR
DIRLIST=realmode
!endif

WIN32=TRUE
NAME=regedit
ROOT=..\..\..\..
RES_DIR=..

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

PRIVINC=pch

PCHOBJ0=regedit.obj regmain.obj regmisc.obj \
        regkey.obj regvalue.obj \
        regbined.obj regstred.obj regdwded.obj \
        regcdhk.obj regfile.obj regporte.obj \
        regprint.obj regnet.obj \
        regfind.obj \
        regdebug.obj

CFLAGS = $(CFLAGS) -Oi

!IF "$(DBCS)" != ""
CFLAGS = $(CFLAGS) -DDBCS
!ENDIF


LIB0=user32.lib kernel32.lib gdi32.lib shell32.lib comctl32.lib advapi32.lib comdlg32.lib

NOMERGEBSS = TRUE

DEFENTRY = ModuleEntry@0

L32FLAGS = $(L32FLAGS) -stub:..\realmode\retail\regdump.exe

DEPENDS = $(DEPENDS) $(SRCDIR)\realmode\retail\regdump.exe

!include $(ROOT)\win\shell\common.mk

!ifdef VERDIR

!include depend.mk

$(PRIVINC).pch pch.obj: $(SRCDIR)\$(PRIVINC).h \
    $(SRCDIR)\regporte.h $(SRCDIR)\regmisc.h $(SRCDIR)\regdebug.h

$(NAME).res:    $(SRCDIR)\$(NAME).ico $(SRCDIR)\regresid.h

!endif

!endif # NTMAKEENV
