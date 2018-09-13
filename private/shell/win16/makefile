##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991-96
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

clean :

!else  # NTMAKEENV


DIR             = win16
ROOT            = ..\..
DIRLIST         = commctrl shell16 

!include ..\shell.mk

default all:    $(DIRLIST)

!endif # NTMAKEENV
