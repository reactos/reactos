!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF


USE_NT_PRODUCT_VER=1

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc 

# Need some NT 5 identifiers
WIN32_WINNT_VERSION=0x0500
WIN32_WIN95_VERSION=0x0500

USE_CRTDLL=1

TARGETNAME=docprop
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=DllEntry
TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32p.lib \
           $(BASEDIR)\public\sdk\lib\*\user32p.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
           $(BASEDIR)\public\sdk\lib\*\shlwapi.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32p.lib \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib


DLLDEF=..\docprop.def

SOURCES= \
    ..\docprop.rc \
    ..\debug.c    \
    ..\offglue.c  \
    ..\propdlg.c  \
    ..\propio.c   \
    ..\propmisc.c \
    ..\sinfo.c   \
    ..\docsum.c  \
    ..\userdef.c \
    ..\docprop.c \
    ..\propvar.c


PRECOMPILED_INCLUDE=..\priv.h
PRECOMPILED_PCH=priv.pch
PRECOMPILED_OBJ=priv.obj
