#//----------------------------------------------------------------------------
#// Build the stub control panel
#//----------------------------------------------------------------------------

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

NAME=control
ROOT=..\..\..\..
RES_DIR=..
# // The 16bit version will be smaller and faster.
# // WIN32=TRUE	
# // DEFENTRY = ModuleEntry@0

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

PCHOBJ0=init.obj 


!ifdef WIN32
!if "$(VERDIR)" == "debug"
#// Debug needs comctl32 only for DebugMsg()
LIB0=kernel32.lib user32.lib comctl32.lib
!else
LIB0=kernel32.lib user32.lib
!endif
!else
!if "$(VERDIR)" == "debug"
#// Debug needs commctrl only for DebugMsg()
LIB0=libw mnocrtw commctrl
!else
LIB0=libw mnocrtw
!endif
!endif

!include $(ROOT)\win\core\shell\common.mk

!ifdef VERDIR
$(PRIVINC).pch pch.obj:	 $(ROOT)\dev\inc16\shell.h

$(NAME).res:	$(SRCDIR)\$(NAME).ico

!endif

!endif # NTMAKEENV
