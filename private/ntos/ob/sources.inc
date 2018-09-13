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
MINORCOMP=ob

TARGETNAME=ob
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;$(BASEDIR)\private\inc
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\obinit.c   \
        ..\obcreate.c \
        ..\obhandle.c \
        ..\obinsert.c \
        ..\obref.c    \
        ..\obse.c     \
        ..\obtype.c   \
        ..\obdir.c    \
        ..\obdevmap.c \
        ..\oblink.c   \
        ..\obclose.c  \
        ..\obquery.c  \
        ..\obsdata.c  \
        ..\obwait.c

PRECOMPILED_INCLUDE=..\obp.h
PRECOMPILED_PCH=obp.pch
PRECOMPILED_OBJ=obp.obj

OPTIONAL_NTTEST=tob
UMTEST=uob

SOURCES_USED=..\sources.inc
