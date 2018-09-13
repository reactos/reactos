!IF 0

Copyright (c) 1989-1996 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

#
# Cross-compiler macros for unique naming
#

MAJORCOMP       = shell
MINORCOMP       = autorun

# relative path to root of shell project
CCSHELL_DIR     = ..\..\..

#
# Common settings required by build.exe and makefile.def
#

INCLUDES=$(BASEDIR)\public\sdk\inc;$(BASEDIR)\private\windows\inc;;$(BASEDIR)\private\inc;$(CCSHELL_DIR)\inc

!if $(386)
CHICAGO_LIB_PATH=$(BASEDIR)\public\sdk\lib\chicago\*
SHELL32_LIB_FILE=$(BASEDIR)\private\shell\lib\i386\shell32.w95
!else
CHICAGO_LIB_PATH=$(SDK_LIB_PATH)
SHELL32_LIB_FILE=$(SDK_LIB_PATH)\shell32.lib
!endif

TARGETNAME = autorun
TARGETPATH = obj
TARGETTYPE = PROGRAM

# msadek; DO NOT link with msvcrt.lib as it will cause problems
# with win95 and winnt3.51

TARGETLIBS= \
    $(CHICAGO_LIB_PATH)\gdi32.lib         \
    $(SHELL32_LIB_FILE)                   \

UMTYPE=windows
UMENTRY=winmain

NOT_LEAN_AND_MEAN=1
SUBSYSTEM_VERSION=4.00
CHICAGO_PRODUCT=1

#
# List of sources
#

SOURCES = \
    ..\autorun.cpp \
    ..\dataitem.cpp \
    ..\dlgapp.cpp  \
    ..\resource.rc \

#
# Separate versions are built for server and professional
#
!IF "$(SERVER)" == "1"
C_DEFINES=$(C_DEFINES) -DBUILD_SERVER_VERSION
!ELSEIF "$(SERVER)" == "2"
C_DEFINES=$(C_DEFINES) -DBUILD_ADVANCED_SERVER_VERSION
!ELSEIF "$(SERVER)" == "3"
C_DEFINES=$(C_DEFINES) -DBUILD_DATACENTER_VERSION
!endif

#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) -DPROFILE
USER_C_FLAGS=$(USER_C_FLAGS) /Gh /Zi
TARGETLIBS=$(TARGETLIBS) $(SDK_LIB_PATH)\icap.lib
!ENDIF

