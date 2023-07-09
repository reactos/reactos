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
    Created 22-Nov-96 by Jeff Saathoff (jeffreys)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=  shell
MINORCOMP=  security

TARGETNAME= common
TARGETTYPE= LIBRARY
TARGETPATH= $(SECLIB_PATH)
TARGETLIBS=

# Aclui.dll, rshx32.dll and dssec.dll depend on common.lib
SYNCHRONIZE_BLOCK=1

CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) \
    shellp.h    \
    shlobjp.h

SOURCES=    ..\debug.cpp     \
            ..\unknown.cpp   \
            ..\strings.cpp   \
            ..\priv.cpp      \
            ..\msgpopup.cpp  \
            ..\misc.cpp      \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
