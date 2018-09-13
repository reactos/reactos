# Makefile : Build the 32-bit local user management DLL.
##########################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#
##########################################################################

!if "$(NTMAKEENV)" != ""
#
# This section is for the NT build ONLY.
# For NT build, change the 'sources' or 'dirs' file, not this makefile
#
UMBASE=0x7db80000

!INCLUDE $(NTMAKEENV)\makefile.def

!else
#
# Non-NT build makefile start HERE.
#

#CODEVIEW=1
WANT_C1032=TRUE
NAME=mslocusr
L32ENTRY=DllEntryPoint@12
DEPENDNAME=depend32.mk
ROOT=..\..\..\..
IS_32=TRUE
IS_SDK=TRUE
IS_DDK=TRUE

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

MISCOBJ0=pch.obj
MISCOBJ1=msluguid.obj

PCHOBJ0=mslocusr.obj msluobj.obj msludb.obj msluuser.obj msluenum.obj mslunp.obj \
	settings.obj

LIB0=$(ROOT)\inet\ohare\ratings\common\retail32\npcommon.lib $(ROOT)\net\user\common\lib\mspwl32.lib
LIB1=$(ROOT)\net\user\common\lib\rsalib.lib
LIB2=kernel32.lib gdi32.lib user32.lib advapi32.lib comdlg32.lib mpr.lib  comctl32.lib 
### BUGBUG - remove libcmt.lib when huge stack hogs are removed
### (__chkstk is the only thing we're pulling in from CRT)
LIB3=$(ROOT)\dev\tools\c932\lib\libcmt.lib

DEPENDLIBS=$(LIB0)

COMMON=$(ROOT)\common

BUILDDLL=TRUE
MKPUBLIC=TRUE

INCFLAGS=-P$$(COMMON)=$(COMMON)

L32BASE=0x7db80000

!include $(ROOT)\inet\ohare\ratings\common32.mk

$(NAME).res:	$(RES_DIR)\logonui.rc $(RES_DIR)\bitmap1.bmp

INCLUDE = $(ROOT)\net\user\common\h;$(INCLUDE)

!endif    # !if NTMAKEENV
