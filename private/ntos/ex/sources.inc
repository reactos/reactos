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
MINORCOMP=ex

TARGETNAME=ex
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;..\..\ke;..\..\kd64

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\callback.c  \
        ..\callperf.c  \
        ..\dbgctrl.c   \
        ..\ddkresrc.c  \
        ..\delay.c     \
        ..\event.c     \
        ..\eventpr.c   \
        ..\exatom.c    \
        ..\exdata.c    \
        ..\exinfo.c    \
        ..\exinit.c    \
        ..\exdata.c    \
        ..\handle.c    \
        ..\harderr.c   \
        ..\lookasid.c  \
        ..\luid.c      \
        ..\memprint.c  \
        ..\mutant.c    \
        ..\pool.c      \
        ..\probe.c     \
        ..\profile.c   \
        ..\raise.c     \
        ..\resource.c  \
        ..\semphore.c  \
        ..\sysenv.c    \
        ..\sysinfo.c   \
        ..\systime.c   \
        ..\timer.c     \
        ..\worker.c    \
        ..\zone.c      \
        ..\uuid.c      \
        ..\win32.c     \
        ..\intbits.c

NTTEST=

PRECOMPILED_INCLUDE=..\exp.h
PRECOMPILED_PCH=exp.pch
PRECOMPILED_OBJ=exp.obj

SOURCES_USED=..\sources.inc
