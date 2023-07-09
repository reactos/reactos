##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

clean :

!else  # NTMAKEENV


DIR=shelldll
ROOT = ..\..\..

DIRLIST=win95

!include ..\..\shell.mk

default all:    $(DIRLIST)

retail debug:
        cd win95
        nmake $@
        
!endif # NTMAKEENV
