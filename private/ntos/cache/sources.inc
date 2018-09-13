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
MINORCOMP=cache

TARGETNAME=cache
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\cachedat.c   \
        ..\cachesub.c   \
        ..\copysup.c    \
        ..\fssup.c      \
        ..\lazyrite.c   \
        ..\logsup.c     \
        ..\mdlsup.c     \
        ..\pinsup.c     \
        ..\vacbsup.c

PRECOMPILED_INCLUDE=..\cc.h
PRECOMPILED_PCH=cc.pch
PRECOMPILED_OBJ=cc.obj

SOURCES_USED=..\sources.inc
