!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include $(CCSHELL_DIR)\ext\mydocs\common\comlib.inc

NO_BROWSER_FILE = 1

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\ext\mydocs\common\comlib.inc

MAJORCOMP=  mydocs
MINORCOMP=  common

TARGETNAME=common
TARGETPATH=$(CCSHELL_DIR)\ext\mydocs\lib\$(MYLIBDIR)
TARGETTYPE=LIBRARY

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF

TARGETLIBS= $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
            $(BASEDIR)\public\sdk\lib\*\user32.lib      \
            $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib

INCLUDES=   $(CCSHELL_DIR)\shell32; \
            $(CCSHELL_DIR)\inc; \
            $(BASEDIR)\private\windows\inc; \
            $(BASEDIR)\private\windows\inc16; \
            $(CCSHELL_DIR)\ext\mydocs\inc; \
            $(CCSHELL_DIR)\ext\mydocs\src; \
            $(CCSHELL_DIR)\ext\mydocs\common

SOURCES=    ..\debug.cpp   \
            ..\unknown.cpp \
            ..\dbmem.cpp   \
            ..\strings.cpp

C_DEFINES = $(C_DEFINES)

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


