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
MINORCOMP=rtl

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                            \
        ..\..\..\inc;                      \
        ..\..\..\inc\$(ALT_PROJECT);       \
        ..\..\..\kernel\$(ALT_PROJECT);    \
        ..\..\..\..\w32inc;                \
        $(BASEDIR)\public\sdk\inc\hydra;   \
        $(BASEDIR)\private\ntos\inc;       \
        $(BASEDIR)\private\ntos\w32\kmode; \
        $(BASEDIR)\private\ntos\w32\ntgdi\inc
	
C_DEFINES=$(C_DEFINES) /DUNICODE /D_UNICODE

SOURCES=..\alignrec.c \
        ..\chartran.c \
        ..\draw.c     \
        ..\drawtext.c \
        ..\getset.c   \
        ..\help.c     \
        ..\input.c    \
        ..\menu.c     \
        ..\mmrtl.c    \
        ..\random.c   \
        ..\rect.c     \
        ..\text.c     \
        ..\winmgr.c   \
        ..\winprop.c  \
        ..\wow.c

PRECOMPILED_INCLUDE=..\..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\..\sources.inc ..\sources.inc
