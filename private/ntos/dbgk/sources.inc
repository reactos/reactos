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
MINORCOMP=dbgk

TARGETNAME=dbgk
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc
GPSIZE=32

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\dbgkport.c \
        ..\dbgkproc.c

UMTEST=udbgk

SOURCES_USED=..\sources.inc
