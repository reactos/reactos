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
MINORCOMP=lpc

TARGETNAME=lpc
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\lpcinit.c  \
        ..\lpccreat.c \
        ..\lpclistn.c \
        ..\lpcconn.c  \
        ..\lpccompl.c \
        ..\lpcsend.c  \
        ..\lpcrecv.c  \
        ..\lpcreply.c \
        ..\lpcquery.c \
        ..\lpcqueue.c \
        ..\lpcclose.c \
        ..\lpcpriv.c

PRECOMPILED_INCLUDE=..\lpcp.h
PRECOMPILED_PCH=lpcp.pch
PRECOMPILED_OBJ=lpcp.obj

UMTYPE=console
UMTEST=userver*uclient

NTTARGETFILES=

SOURCES_USED=..\sources.inc
