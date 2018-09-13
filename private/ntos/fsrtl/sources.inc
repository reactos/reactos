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
MINORCOMP=FsRtl

TARGETNAME=FsRtl
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc
GPSIZE=32

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\DbcsName.c \
        ..\FastIo.c   \
        ..\FaultTol.c \
        ..\FileLock.c \
        ..\Filter.c   \
        ..\FiltrCtx.c \
        ..\FsRtlPc.c  \
        ..\LargeMcb.c \
        ..\Name.c     \
        ..\Notify.c   \
        ..\Oplock.c   \
        ..\PnP.c      \
        ..\Unc.c      \
        ..\Tunnel.c   \
        ..\StackOvf.c

PRECOMPILED_INCLUDE=..\fsrtlp.h
PRECOMPILED_PCH=fsrtlp.pch
PRECOMPILED_OBJ=fsrtlp.obj

SOURCES_USED=..\sources.inc
