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
MINORCOMP=imm


TARGETNAME=imm32
TARGETTYPE=DYNLINK
DLLENTRY=ImmDllInitialize

TARGETPATH=obj\$(ALT_PROJECT_TARGET)
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib                       \
           $(BASEDIR)\public\sdk\lib\*\gdi32p.lib   \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib                       \
           $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\user32p.lib

LINKLIBS = $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\w32umode.lib \
           $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\user32p.lib \
           ..\..\obj\$(ALT_PROJECT_TARGET)\*\userrtl.lib \
           ..\..\..\obj\*\w32crtl.lib

# LINKER_FLAGS = $(LINKER_FLAGS) -calltree

NTTARGETFILE1=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\imm32p.lib

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE

SOURCES_OPTIONS=-c0

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

WINDIR=$(BASEDIR)\private\windows
INCLUDES=..;..\..\inc;..\..\inc\$(ALT_PROJECT);..\..\..\w32inc;$(WINDIR)\inc;$(BASEDIR)\private\ntos\inc;..\..\..\ntgdi\inc

#
NTPROFILEINPUT=yes

#
# Setup for application developer to set breakpoints with windbg
#

# NTDEBUGTYPE=coff

NT_UP=0
USE_NTDLL=1
DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

SOURCES=..\res.rc     \
        ..\context.c  \
        ..\ctxtinfo.c \
        ..\globals.c  \
        ..\hotkey.c   \
        ..\immime.c   \
        ..\imminit.c  \
        ..\layime.c   \
        ..\layout.c   \
        ..\misc.c     \
        ..\regword.c  \
        ..\sftkbdc1.c \
        ..\sftkbdt1.c \
        ..\softkbd.c  \
        ..\input.c    \
        ..\clwinnls.c \
        ..\transsub.c \
        ..\kcodecnv.c \
        ..\jtranmsg.c \
        ..\ktranmsg.c \
        ..\conime.c \
        ..\imemenu.c

UMTYPE=windows
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
