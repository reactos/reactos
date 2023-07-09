##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996
#       All Rights Reserved.
#
##########################################################################

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

clean :

!else  # NTMAKEENV

DIR             = ext
ROOT            = ..\..

DIRLIST         = favorite ftp zip docprop shscrap viruschk

!include ..\shell.mk

default all:    $(DIRLIST)

!endif # NTMAKEENV
