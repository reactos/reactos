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

MAJORCOMP=user
MINORCOMP=userexts

TARGETNAME=userexts
TARGETTYPE=DYNLINK

TARGETPATH=obj

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\user32p.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib

DELAYLOAD=user32.dll

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                            \
        ..\..\..\client;                   \
        ..\..\..\inc;                      \
        ..\..\..\inc\$(ALT_PROJECT);       \
        ..\..\..\server;                   \
        $(BASEDIR)\private\ntos\inc;       \
        $(BASEDIR)\private\ntos\w32\ntgdi\inc; \
        $(BASEDIR)\private\ntos\w32\w32inc; \
        $(BASEDIR)\private\ntos\w32\ntcon\inc




USE_NTDLL=1

SOURCES=..\userexts.c  \
        ..\userexts.rc

SOURCES_OPTIONS=-c0
NTTARGETFILE0=userexts.def ..\$(TARGET_DIRECTORY)\dsotable.c
UMTYPE=console
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
