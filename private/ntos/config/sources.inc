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
MINORCOMP=config

TARGETNAME=config
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc

GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\cmapi.c     \
        ..\cmapi2.c    \
        ..\cmboot.c    \
        ..\cmchek.c    \
        ..\cmchek2.c   \
        ..\cmclose.c   \
        ..\cmconfig.c  \
        ..\cmcontrl.c  \
        ..\cmdat.c     \
        ..\cmdat2.c    \
        ..\cmdat3.c    \
        ..\cmdatini.c  \
        ..\cmdelete.c  \
        ..\cmgquota.c  \
        ..\cmhvlist.c  \
        ..\cmindex.c   \
        ..\cminit.c    \
        ..\cmname.c    \
        ..\cmnotify.c  \
        ..\cmparse.c   \
        ..\cmparse2.c  \
        ..\cmquery.c   \
        ..\cmsavres.c  \
        ..\cmsubs.c    \
        ..\cmsubs2.c   \
        ..\cmsubs3.c   \
        ..\cmse.c      \
        ..\cmsysini.c  \
        ..\cmtrecpy.c  \
        ..\cmtredel.c  \
        ..\cmtree.c    \
        ..\cmworker.c  \
        ..\cmwrapr.c   \
        ..\cmwrapr2.c  \
        ..\cmwmi.c     \
        ..\hivebin.c   \
        ..\hivecell.c  \
        ..\hivechek.c  \
        ..\hivefree.c  \
        ..\hiveinit.c  \
        ..\hiveload.c  \
        ..\hivemap.c   \
        ..\hivesum.c   \
        ..\hivesync.c  \
        ..\ntapi.c     \
        ..\hwprofil.c

NTTEST=
UMTYPE=console
UMTEST=

PRECOMPILED_INCLUDE=..\cmp.h
PRECOMPILED_PCH=cmp.pch
PRECOMPILED_OBJ=cmp.obj

SOURCES_USED=..\sources.inc
