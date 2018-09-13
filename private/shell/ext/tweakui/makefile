!ERROR WRONG SOURCE TREE - USE \\ORVILLE\RAZZLE\SRC\SDKTOOLS\TWEAKUI
#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1995-1996
#	All Rights Reserved.
#
#	Makefile for TWEAKUI
#
#############################################################################

!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!else

# We must be Win95-compatible
BLDPROJ=OPK2

!IFDEF BLDROOT
ROOT=$(BLDROOT)
!else
ROOT=c:\win32
!endif

VERSIONLIST = retail debug
IS_32 = TRUE
IS_SDK = TRUE
IS_PRIVATE = TRUE			# IShellView is internal
COMMONMKFILE = TWEAKUI.MK

!include $(ROOT)\dev\master.mk

!endif
