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

!include $(CCSHELL_DIR)\ext\settings\settings.inc

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\ext\settings\settings.inc

INDENTED_DIRECTIVES=1

MAJORCOMP=shell
MINORCOMP=SettingsAssistant

USE_CRTDLL=1
USE_NATIVE_EH=1
TARGETNAME=settings
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
URES=obj\*\settings.res
DLLDEF=..\settings.def
DLLENTRY=_DllMainCRTStartup
#
# Define macros to correctly place products from MC (msg.h, msg.rc etc).
#
IDL_TYPE=rpc
PASS0_HEADERDIR=..
PASS0_CLIENTDIR=..
TARGETLIBS=\
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\user32.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\comctlp.lib  \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32p.lib


INCLUDES=..;\
         $(BASEDIR)\private\shell\shell32;\
         $(BASEDIR)\private\shell\inc;\
         $(BASEDIR)\private\windows\inc;\
         $(BASEDIR)\private\windows\inc16;\
         $(BASEDIR)\private\inc

SOURCES= ..\msg.mc \
         ..\alloc.cpp \
         ..\comobj.cpp \
         ..\debug.cpp \
         ..\dynarray.cpp \
         ..\folder.cpp \
         ..\globals.cpp \
         ..\gdiobj.cpp \
         ..\guids.cpp \
         ..\main.cpp \
         ..\stdext.cpp \
         ..\utils.cpp \
         ..\settings.rc 

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
PRECOMPILED_FLAG=/GX
