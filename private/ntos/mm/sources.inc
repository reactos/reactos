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
MINORCOMP=mm

TARGETNAME=mm
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;..\..\ke;$(BASEDIR)\private\inc
MIPS_OPTIONS=-nodwalign
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\acceschk.c \
        ..\addrsup.c  \
        ..\allocpag.c \
        ..\allocvm.c  \
        ..\checkpfn.c \
        ..\checkpte.c \
        ..\creasect.c \
        ..\deleteva.c \
        ..\dmpaddr.c  \
        ..\dynmem.c   \
        ..\extsect.c  \
        ..\flushbuf.c \
        ..\flushsec.c \
        ..\forksup.c  \
        ..\freevm.c   \
        ..\iosup.c    \
        ..\lockvm.c   \
        ..\mapcache.c \
        ..\mapview.c  \
        ..\miglobal.c \
        ..\mmfault.c  \
        ..\mminit.c   \
        ..\mmsup.c    \
        ..\mmquota.c  \
        ..\modwrite.c \
        ..\pagfault.c \
        ..\pfndec.c   \
        ..\pfnlist.c  \
        ..\physical.c \
        ..\procsup.c  \
        ..\protect.c  \
        ..\querysec.c \
        ..\queryvm.c  \
        ..\readwrt.c  \
        ..\sectsup.c  \
        ..\session.c  \
        ..\sessload.c \
        ..\shutdown.c \
        ..\sysload.c  \
        ..\sysptes.c  \
        ..\triage.c   \
        ..\umapview.c \
        ..\vadtree.c  \
        ..\verifier.c \
        ..\wslist.c   \
        ..\wsmanage.c \
        ..\wstree.c   \
        ..\wrtfault.c \
        ..\wrtwatch.c \
        ..\zeropage.c

PRECOMPILED_INCLUDE=..\mi.h
PRECOMPILED_PCH=mi.pch
PRECOMPILED_OBJ=mi.obj

SOURCES_USED=..\sources.inc
