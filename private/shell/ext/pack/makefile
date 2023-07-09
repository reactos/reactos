#
# This is the makefile for the old structure on \\trango
#
#

!ifdef NTMAKEENV

clean:

!else # NTMAKEENV

NAME=pack2
ROOT=..\..\..\..
RES_DIR=..
WIN32=TRUE
OLENOT=TRUE
DEFNAME=pack2.def

RESNAME=pack2.res
RCNAME=pack2.rc
RCVNAME=pack2.rcv
FORCE_CPP=-Tp

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

DLLENTRY = LibMain

PCHOBJ0 = pack2.obj ioleobj.obj idataobj.obj iviewobj.obj iperstor.obj iadvsink.obj \
         irunobj.obj debug.obj pack2cls.obj packutil.obj

LIB0 = kernel32.lib user32.lib gdi32.lib  ole32.lib uuid.lib \
       shell32.lib libcmt.lib advapi32.lib shlwapi.lib shlwapip.lib foo shellp.lib

BUILDDLL=TRUE

!include $(ROOT)\win\shell\common.mk

$(SRCDIR)\pack2.def: $(SRCDIR)\pack2.src
        cl -EP $(SRCDIR)\pack2.src > $(SRCDIR)\pack2.def

pack2.res:

!endif # NTMAKEENV