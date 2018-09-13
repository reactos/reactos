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
!INCLUDE $(NTMAKEENV)\makefile.def

!else
#
# Non-NT build makefile start HERE.
#

CODEVIEW=1
WANT_C1032=TRUE
NAME=msrating
L32ENTRY=DllEntryPoint@12
DEPENDNAME=..\depend32.mk
ROOT=..\..\..\..\..
IS_32=TRUE
IS_SDK=TRUE
IS_DDK=TRUE

!ifdef VERDIR
ROOT=..\..\$(ROOT)
!endif

MISCOBJ0=..\pch.obj
MISCOBJ1=..\ratguid.obj

PCHOBJ0=..\msrating.obj ..\roll.obj ..\rors.obj ..\rocycle.obj ..\mslubase.obj \
        ..\ratings.obj ..\superpw.obj ..\msludlg.obj ..\parselbl.obj \
        ..\picsuser.obj ..\comobj.obj ..\parserat.obj ..\filedlg.obj

LIB0=$(ROOT)\inet\ohare\ratings\common\retail32\npcommon.lib
LIB1=$(ROOT)\net\user\common\lib\rsalib.lib 
# Use the below to get internal versions of Tls functions
LIB2=kernel32.lib gdi32.lib user32.lib advapi32.lib uuid.lib comctl32.lib
### BUGBUG - remove libcmt.lib when huge stack hogs are removed
### (__chkstk is the only thing we're pulling in from CRT)
LIB3=libcmt.lib 

DEPENDLIBS=$(LIB0)

COMMON=$(ROOT)\common

BUILDDLL=TRUE
MKPUBLIC=TRUE

INCFLAGS=-P$$(COMMON)=$(COMMON)

L32BASE=0x7db00000
CUSTOMCFLAGS=-GF -Oxs -Gy

!include $(ROOT)\inet\ohare\ratings\common32.mk

INCLUDE = $(ROOT)\net\user\common\h;$(INCLUDE);$(ROOT)\inet\ohare\inc

!endif    # !if NTMAKEENV
