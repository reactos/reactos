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
MINORCOMP=lfs

TARGETNAME=lfs
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc

C_DEFINES=$(C_DEFINES) -D_NTDRIVER_

!IFDEF BUILD_FOR_3_51
C_DEFINES= $(C_DEFINES) -D_NTIFS_
!ENDIF

MSC_WARNING_LEVEL=/W3 /WX



SOURCES=..\CacheSup.c  \
        ..\LbcbSup.c   \
        ..\LfsData.c   \
        ..\LogPgSup.c  \
        ..\LogRcSup.c  \
        ..\LsnSup.c    \
        ..\QueryLog.c  \
        ..\Registry.c  \
        ..\Restart.c   \
        ..\RstrtSup.c  \
        ..\StrucSup.c  \
        ..\SysInit.c   \
        ..\VerfySup.c  \
        ..\Write.c

PRECOMPILED_INCLUDE=..\lfsprocs.h
PRECOMPILED_PCH=lfsprocs.pch
PRECOMPILED_OBJ=lfsprocs.obj
