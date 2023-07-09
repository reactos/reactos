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
MINORCOMP=se

TARGETNAME=se
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;$(BASEDIR)\private\inc
MIPS_OPTIONS=-nodwalign
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=-D_NTSYSTEM_

SOURCES=              \
        ..\accessck.c \
        ..\capture.c  \
        ..\privileg.c \
        ..\rmlogon.c  \
        ..\rmmain.c   \
        ..\rmvars.c   \
        ..\seassign.c \
        ..\seaudit.c  \
        ..\sepaudit.c \
        ..\seclient.c \
        ..\seglobal.c \
        ..\seinit.c   \
        ..\semethod.c \
        ..\sep.c      \
        ..\subject.c  \
        ..\seastate.c \
        ..\token.c    \
        ..\tokenadj.c \
        ..\tokendup.c \
        ..\tokenopn.c \
        ..\tokenqry.c \
        ..\tokenset.c \
        ..\adtlog.c   \
        ..\adtinit.c  \
        ..\adtvars.c  \
        ..\rmaudit.c

SOURCES_USED=..\sources.inc

! IF 0

UMTEST=uttoken
UMTYPE=console
UMLIBS=$(BASEDIR)\public\sdk\lib\*\ntdll.lib

!ENDIF
