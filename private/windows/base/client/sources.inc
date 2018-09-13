!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Mark Lucovsky (markl) 18-Sep-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=windows
MINORCOMP=base

TARGETNAME=kernel32
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETPATHLIB=obj
TARGETTYPE=DYNLINK
W32_SB=1

NTTARGETFILES= \
    $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernl32p.lib \
    $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernel32.lib

DLLDEF=obj\*\kernel32.def

LINKLIBS=\
    $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\conlibk.lib \
    ..\..\..\obj\$(ALT_PROJECT_TARGET)\*\nlslib.lib \
    $(BASEDIR)\public\sdk\lib\*\dload.lib \
    $(BASEDIR)\public\sdk\lib\*\ntdll.lib


DLLENTRY=BaseDllInitialize

USE_NTDLL=1

INCLUDES=..\;..\..\inc;..\..\..\inc;..\..\..\..\inc

CONDITIONAL_INCLUDES=doswin32.h

C_DEFINES=$(C_DEFINES) -D_KERNEL32_

MSC_WARNING_LEVEL=/W3 /WX

SYNCHRONIZE_BLOCK=1

DLLORDER=..\kernel32.prf

SOURCES= ..\base.rc    \
         ..\backup.c   \
         ..\beep.c     \
         ..\compname.c \
         ..\curdir.c   \
         ..\datetime.c \
         ..\debug.c    \
         ..\dir.c      \
         ..\dllatom.c  \
         ..\dllini.c   \
         ..\dosdev.c   \
         ..\error.c    \
         ..\filefind.c \
         ..\filehops.c \
         ..\filemap.c  \
         ..\filemisc.c \
         ..\fileopcr.c \
         ..\gmem.c     \
         ..\handle.c   \
         ..\baseinit.c \
         ..\lcompat.c  \
         ..\lmem.c     \
         ..\mailslot.c \
         ..\message.c  \
         ..\module.c   \
         ..\namepipe.c \
         ..\pathmisc.c \
         ..\perfctr.c  \
         ..\pipe.c     \
         ..\power.c    \
	 ..\process.c  \
         ..\sessdev.c  \
         ..\support.c  \
         ..\synch.c    \
         ..\termutil.c \
         ..\thread.c   \
         ..\toolhelp.c \
         ..\ustubs.c   \
         ..\comm.c     \
         ..\vdm.c      \
         ..\volmount.c \
         ..\updres.c   \
         ..\blddcb.c   \
         ..\tapeapi.c  \
         ..\job.c      \
         ..\winlinks.c \
         ..\w98stubs.c

UMTYPE=console
OPTIONAL_UMTEST=tatom*tprof*taskstrs*winperf*tbase*tprof*tfile*ttask*tmem*tlfn

PRECOMPILED_INCLUDE=..\basedll.h
PRECOMPILED_PCH=basedll.pch
PRECOMPILED_OBJ=basedll.obj
