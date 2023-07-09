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
MINORCOMP=ioe

TARGETNAME=ioe
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc;..\..\io
MIPS_OPTIONS=-nodwalign
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\ioeapi.c     \
        ..\ioerr.c      \
        ..\data.c       \
        ..\trace.c

NTTEST=
OPTIONAL_NTTEST=

PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

SOURCES_USED=..\sources.inc
