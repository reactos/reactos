#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1995-1996
#	All Rights Reserved.
#
#	Makefile for TWEAKUI
#
#############################################################################

# We must be Win95-compatible
BLDPROJ=OPK2

!IFDEF BLDROOT
ROOT=$(BLDROOT)
!else
ROOT=c:\win32
!endif

SRCDIR=..
IS_32=TRUE
IS_SDK=TRUE
IS_PRIVATE = TRUE			# IShellView is internal
DEPENDNAME=..\depend.mk
WANT_C1032=TRUE
BUILD_COFF=TRUE

BUILDDLL=TRUE

L32EXE=TWEAKUI.DLL
L32RES=TWEAKUI.RES
L32DEF=$(SRCDIR)\TWEAKUI.DEF
L32MAP=TWEAKUI.MAP

#
#	"What I say three times is true."
#	    -- Lewis Carroll, "The Hunting of the Snark"
#
# I hate master.mk
#
DLLENTRY=Entry32
DEFENTRY=Entry32
L32FLAGS=$(L32FLAGS) -entry:$(DLLENTRY) -def:$(L32DEF)

L32OBJS=\
TWEAKUI.OBJ \
COMMON.OBJ \
GENERAL.OBJ \
MOUSE.OBJ \
EXPLORER.OBJ \
LINK.OBJ \
DESKTOP.OBJ \
CONTROL.OBJ \
NETWORK.OBJ \
MYCOMP.OBJ \
TOOLS.OBJ \
ADDRM.OBJ \
BOOT.OBJ \
REPAIR.OBJ \
PARANOIA.OBJ \
OLE.OBJ \
WITH.OBJ \
PIDL.OBJ \
PICKICON.OBJ \
MISC.OBJ \
LV.OBJ \
LVCHK.OBJ \
REG.OBJ \
EXPIRE.OBJ \
IE4.OBJ \
STRINGS.OBJ \

TARGETS=$(L32EXE)

L32LIBSNODEP=kernel32.lib advapi32.lib user32.lib shell32.lib \
             comctl32.lib comdlg32.lib gdi32.lib version.lib

# I hate includes.exe
#
# Must manually exclude all the random header files that never change.
#
# And it still doesn't generate the dependency for the .rc file properly,
# so
#
#   WARNING WARNING WARNING
#
# After an "nmake depend", append the following lines to depend.mk by hand:
#
#	$(OBJDIR)\tweakui.res: ..\tweakui.rc ..\tweakui.h
#
INCFLAGS=$(INCFLAGS) -nwindows.h -nwindowsx.h -nshellapi.h -nshlobj.h -nregstr.h -ncommdlg.h -ncpl.h -ncommctrl.h -nprsht.h -n..\inc16\shsemip.h

L32FLAGS=$(L32FLAGS) -base:0x40000000

!include $(ROOT)\dev\master.mk

INCLUDE=$(ROOT)\win\core\shell\inc;$(INCLUDE)
CFLAGS=$(CFLAGS) -YX -Zp1 -Oxs -W3 -WX -Gz
