#----------------------------------------------------------------------------
# Build the progman group converter
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

NAME=grpconv
ROOT=..\..\..\..
RES_DIR=..
WIN32=TRUE
DEFENTRY = ModuleEntry@0

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

PCHOBJ0=init.obj util.obj group.obj gcinst.obj
LIB0=user32.lib kernel32.lib gdi32.lib shell32.lib comctl32.lib advapi32.lib version.lib

!include $(ROOT)\win\core\shell\common.mk

!ifdef VERDIR
$(PRIVINC).pch pch.obj:  $(ROOT)\dev\inc16\shell.h

$(NAME).res:    $(SRCDIR)\$(NAME).ico $(SRCDIR)\welcome.bmp $(SRCDIR)\rcids.h

!endif

!endif # NTMAKEENV
