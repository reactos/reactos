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

MAJORCOMP=ntos
MINORCOMP=ke

TARGETNAME=ke
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc

!if $(AXP64) || $(ALPHA)
INCLUDES=$(INCLUDES);$(BASEDIR)\private\ntos\fw\alpha
!endif

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\apcobj.c   \
        ..\apcsup.c   \
        ..\balmgr.c   \
        ..\bugcheck.c \
        ..\channel.c  \
        ..\config.c   \
        ..\debug.c    \
        ..\devquobj.c \
        ..\dpcobj.c   \
        ..\dpcsup.c   \
        ..\eventobj.c \
        ..\kernldat.c \
        ..\kiinit.c   \
        ..\miscc.c    \
        ..\mutntobj.c \
        ..\procobj.c  \
        ..\profobj.c  \
        ..\queueobj.c \
        ..\raisexcp.c \
        ..\semphobj.c \
        ..\thredobj.c \
        ..\thredsup.c \
        ..\timerobj.c \
        ..\timersup.c \
        ..\wait.c     \
        ..\waitsup.c  \
        ..\xipi.c     \
        ..\yield.c

NTTEST=
UMTYPE=console
UMLIBS=$(BASEDIR)\public\sdk\lib\*\user32.lib
UMTEST=

NTTARGETFILES=

PRECOMPILED_INCLUDE=..\ki.h
PRECOMPILED_PCH=ki.pch
PRECOMPILED_OBJ=ki.obj

SOURCES_USED=..\sources.inc
