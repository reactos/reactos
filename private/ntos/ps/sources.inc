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
MINORCOMP=ps

TARGETNAME=ps
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;..\..\ke

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\psspnd.c   \
        ..\pscid.c    \
        ..\psimpers.c \
        ..\psinit.c   \
        ..\psdelete.c \
        ..\psquery.c  \
        ..\create.c   \
        ..\kulookup.c \
        ..\psquota.c  \
        ..\psctx.c    \
        ..\psopen.c   \
        ..\psjob.c    \
        ..\security.c

PRECOMPILED_INCLUDE=..\psp.h
PRECOMPILED_PCH=psp.pch
PRECOMPILED_OBJ=psp.obj

UMTYPE=nt
UMTEST=

SOURCES_USED=..\sources.inc
