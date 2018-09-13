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
    Created 21-Nov-97 by Brian Aust (brianau)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=csc
MINORCOMP=cscui

TARGETNAME=viewer
TARGETPATH=$(CSCUILIB_PATH)
TARGETTYPE=LIBRARY

USE_CRTDLL=1
USE_NATIVE_EH=1

TARGETLIBS=
LINKLIBS=

INCLUDES=$(INCLUDES)\
    $(BASEDIR)\public\sdk\inc\crt;

SOURCES=\
    ..\alloc.cpp    \
    ..\filesize.cpp \
    ..\registry.cpp \
    ..\strclass.cpp \
    ..\openfile.cpp \
    ..\pathstr.cpp  \
    ..\utils.cpp    \
    ..\options.cpp  \
    ..\debug.cpp    \
    ..\config.cpp   \
    ..\strret.cpp   \
    ..\purge.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
