!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=gdi
MINORCOMP=cmm_open

!ifndef ICMROOT
ICMROOT=$(BASEDIR)\private\ntos\w32\ntgdi\icm
!endif

!include $(ICMROOT)\icmsrc.inc

TARGETNAME=lh_open
TARGETTYPE=LIBRARY
TARGETPATH=..\$(ALT_PROJECT_TARGET)\obj

INCLUDES=..\.;..\..\lh_core

C_DEFINES=$(C_DEFINES) -DWIN32 -D__MSDOS__ -DIntelMode -DRenderInt -DMS_Icm

MSC_WARNING_LEVEL=/W3 /WX

SOURCES= ..\pi_cmm.c   \
         ..\pi_color.c \
         ..\pi_mem.c   \
         ..\pi_run.c   \
         ..\pi_tick.c  \
         ..\pi_valid.c

