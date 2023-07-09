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

CCSHELL_DIR=$(BASEDIR)\private\shell

MAJORCOMP=shell
MINORCOMP=shlexts

TARGETNAME=shlexts
TARGETTYPE=DYNLINK

TARGETPATH=obj

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\user32p.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib \
           $(BASEDIR)\public\sdk\lib\*\version.lib \
           $(CCSHELL_DIR)\lib\$O\shguid.lib              \
           $(CCSHELL_DIR)\lib\$O\shguidp.lib             \
           ..\cpp\$O\shlcpp.lib

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES= ..;                            \
          $(BASEDIR)\private\windows\inc;\
          $(BASEDIR)\shell\inc;          \
          $(BASEDIR)\private\inc;

USE_NTDLL=1

SOURCES=..\shlexts.c  \
        dsotable.c \
        ..\shlexts.rc

SOURCES_OPTIONS=-c0
NTTARGETFILE0=shlexts.def dsotable.c
UMTYPE=console
UMTEST=
UMLIBS=


PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


SOURCES_USED=..\sources.inc
