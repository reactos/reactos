
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
MINORCOMP=server

TARGETNAME=usersrvl
TARGETTYPE=LIBRARY

TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -DNEW_MINLOOK -D_USER32_ -DUSE_MIRRORING
!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

NTTARGETFILES=

INCLUDES=..\;                                   \
         ..\..\inc;                             \
         ..\..\inc\$(ALT_PROJECT);              \
         ..\..\..\w32inc;                       \
         ..\..\client;                          \
         ..\..\..\ntgdi\inc


NTPROFILEINPUT=yes

#
# By setting NTDEBUGTYPE==coff, NTLEANANDMEAN==leanandmean will leave minimal
# debug info in usersrv.dll.  If you don't like long links, set your
# build environment to have NTLEANANDMEAN==smallandfast.
# Whatever you do, DON'T set NTLEANANDMEAN==bigandbusted!
#
NTDEBUGTYPE=coff

SOURCES= \
        ..\debug.c    \
        ..\exitwin.c  \
        ..\exports.c  \
        ..\globals.c  \
        ..\harderr.c  \
        ..\msgbeep.c  \
        ..\instdev.c  \
        ..\server.c

UMTYPE=windows
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
