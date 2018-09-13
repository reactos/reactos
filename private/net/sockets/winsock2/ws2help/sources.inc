!IF 0

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl


!ENDIF

MAJORCOMP=winsock2
MINORCOMP=ws2help

TARGETNAME=ws2help
TARGETTYPE=DYNLINK

TARGETLIBS=                                     \
    $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \

INCLUDES=..\;..\..\inc;..\..\..\..\..\inc

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_WS2HELP_

!IFDEF BUILD_FOR_3_51
C_DEFINES=$(C_DEFINES) -DNT351
!ENDIF

SOURCES=                \
        apc.c           \
        ..\context.c    \
        ..\ws2help.rc   \

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

NTTARGETFILE0=ws2help.def

SOURCES_USED=..\sources.inc

