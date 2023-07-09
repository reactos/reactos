!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

History:
    Created 29-Oct-98 by Brian Aust (brianau)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=csc
MINORCOMP=cscui

TARGETNAME=msg
TARGETPATH=$(CSCUILIB_PATH)
TARGETTYPE=LIBRARY
TARGETLIBS=
LINKLIBS=

SYNCHRONIZE_BLOCK = 1

PASS0_SOURCEDIR=..\..\lib
PASS0_HEADERDIR=..\..\inc

SOURCES= ..\msg.mc
