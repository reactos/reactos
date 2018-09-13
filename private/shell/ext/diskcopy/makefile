!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

ROOT=..\..\..\..
RES_DIR=..
WIN32=TRUE

BUILDDLL=TRUE

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

NAME=DiskCopy

CFLAGS=$(CFLAGS) -Gy

MISCOBJ0=debug.obj 

PCHOBJ0=shellext.obj diskcopy.obj

LIB0=uuid.lib user32.lib gdi32.lib shell32.lib kernel32.lib comctl32.lib libcmt.lib

!include $(ROOT)\win\core\shell\common.mk

!ifdef VERDIR

$(NAME).res:	$(SRCDIR)\diskcopy.ico

!endif

!endif # NTMAKEENV
