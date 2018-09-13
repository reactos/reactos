##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

ROOT        = ..\..\..

MSDEVMK     = folder.mak

CFG_RETAIL  = "folder - Win32 Release" 
CFG_DEBUG   = "folder - Win32 Debug"

!include $(ROOT)\dev\vcwrap.mk

INCLUDE		= ..\..\inc;$(INCLUDE);$(ROOT)\dev\inc;$(ROOT)\dev\msdev\include

!endif