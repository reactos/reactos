!IF 0

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Hirotoshi Shimizu(v-HirShi) 21-Jun-1995

!ENDIF


MAJORCOMP=windows
MINORCOMP=console

TARGETNAME=conime
TARGETPATH=obj
TARGETTYPE=PROGRAM

INCLUDES=..\..\inc;                             \
         ..\..\..\w32inc;                       \
         ..\;

USE_CRTDLL=1

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=-DWIN32 -DNT -DUNICODE -D_UNICODE

SOURCES=..\conime.c \
        ..\consubs.c \
        ..\imefull.c \
        ..\country.c \
        ..\country2.c \
        ..\country3.c \
        ..\conime.rc

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

UMTYPE=windows
UMENTRY=winmain
UMAPPL=conime
UMLIBS=   $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
          $(BASEDIR)\public\sdk\lib\*\kernl32p.lib    \
          $(BASEDIR)\public\sdk\lib\*\user32.lib      \
          $(BASEDIR)\public\sdk\lib\*\ntdll.lib       \
          $(BASEDIR)\public\sdk\lib\*\imm32.lib       \
          $(BASEDIR)\public\sdk\lib\*\imm32p.lib      \
          $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
          $(BASEDIR)\public\sdk\lib\*\shell32.lib

