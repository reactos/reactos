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
MINORCOMP=userkdx

TARGETNAME=userkdx
TARGETTYPE=DYNLINK

TARGETPATH=obj

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib

LINKLIBS=$(BASEDIR)\public\sdk\lib\*\libc.lib

KERNEL=1

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -D_USER32_ -DKERNEL -D_CROSS_PLATFORM_

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                                \
        ..\..\..\kernel;                       \
        ..\..\..\kernel\$(ALT_PROJECT);        \
        ..\..\..\inc;                          \
        ..\..\..\inc\$(ALT_PROJECT);           \
        ..\..\..\..\w32inc;                    \
        ..\..\..\..\ntgdi\inc;                 \
        $(BASEDIR)\private\ntos\inc;           \
        $(BASEDIR)\private\ntos\fsrtl;         \
        $(BASEDIR)\private\ntos\cache;         \
        $(BASEDIR)\private\ntos\cntfs;         \
        $(BASEDIR)\private\ntos\dd\cnss;       \
        $(BASEDIR)\private\ntos\w32\ntgdi\inc; \
        $(BASEDIR)\private\ntos\w32\w32inc;    \
        $(BASEDIR)\private\ntos\w32\ntcon\inc



USE_NTDLL=1

SOURCES=..\kdexts.c    \
        ..\userexts.c  \
        ..\ia64.c      \
        ..\i386.c      \
        ..\alpha.c     \
        ..\userexts.rc

SOURCES_OPTIONS=-c0
NTTARGETFILE0=userkdx.def ..\$(TARGET_DIRECTORY)\dsotable.c
UMTYPE=console

SOURCES_USED=..\sources.inc

SUBSYSTEM_VERSION=4.0
