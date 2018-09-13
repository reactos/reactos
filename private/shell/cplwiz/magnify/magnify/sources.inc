!IF 0

Copyright (c) 1989-1997 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

INDENTED_DIRECTIVES=1
MAJORCOMP=shell
MINORCOMP=magnify

#
# We need to make sure we pick up the right information in the header files
# TODO: Verify that this is the right place for these defines
#

C_DEFINES=$(C_DEFINES)  -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0400



#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
ICECAP_LIBS=$(BASEDIR)\public\sdk\lib\*\icap.lib
USER_C_FLAGS=$(USER_C_FLAGS) -Gh -Zi -MD
C_DEFINES=$(C_DEFINES) -DPROFILE
!ENDIF

TARGETNAME=magnify
TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_CRTDLL=1
USE_NATIVE_EH=1

TARGETLIBS=$(TARGETLIBS) \
           $(ICECAP_LIBS) \
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\user32p.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
           $(BASEDIR)\public\sdk\lib\*\mpr.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
           $(BASEDIR)\public\sdk\lib\*\comdlg32.lib \
           $(BASEDIR)\public\sdk\lib\*\syssetup.lib \
           $(BASEDIR)\public\sdk\lib\*\setupapi.lib \
           $(BASEDIR)\public\sdk\lib\*\version.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib \
           $(BASEDIR)\public\sdk\lib\*\ddraw.lib

#
# nafxcw.lib - JMC fix this   
#           $(BASEDIR)\public\sdk\lib\*\mfc42.lib \
#           $(BASEDIR)\public\sdk\lib\*\msvcrt.lib \
#

NOT_LEAN_AND_MEAN=1

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;\
         $(BASEDIR)\public\sdk\inc;\
         $(BASEDIR)\public\sdk\inc\mfc42;


SOURCES= ..\AppBar.cpp   \
         ..\FastDib.cpp  \
         ..\MagBar.cpp   \
         ..\MagDlg.cpp   \
         ..\Magnify.cpp  \
         ..\ZoomRect.cpp \
         ..\Magnify.rc

!if $(386)
UMENTRYABS      = WinMainCRTStartup
!else
UMENTRYABS      = WinMainCRTStartup
!endif


LINKLIBS=

UMTYPE=windows

PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_CXX=1

# Figure out how to get pre-compiled headers working later
# PRECOMPILED_INCLUDE=..\stdafx.h
# PRECOMPILED_PCH=stdafx.pch
# PRECOMPILED_OBJ=stdafx.obj
# PRECOMPILED_FLAG=

