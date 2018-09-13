!IF 0

Copyright (c) 1989  Microsoft Corporation


!ENDIF

MAJORCOMP=shell
MINORCOMP=control

TARGETNAME=console
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETEXT=dll
DLLENTRY=DllInitialize

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\user32.lib  \
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32p.lib \
           $(BASEDIR)\public\sdk\lib\*\shlwapi.lib


INCLUDES=..\;$(BASEDIR)\private\windows\inc;$(BASEDIR)\private\shell\shell32

C_DEFINES=-DUNICODE -D_UNICODE -D_WIN32

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

UMTYPE=windows

SOURCES=..\console.c  \
        ..\fontdlg.c  \
        ..\init.c     \
        ..\link.c     \
        ..\misc.c     \
        ..\preview.c  \
        ..\registry.c \
        ..\dbcs.c     \
        ..\console.rc

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
