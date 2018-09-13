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
MINORCOMP=accwiz

#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
ICECAP_LIBS=$(BASEDIR)\public\sdk\lib\*\icap.lib
USER_C_FLAGS=$(USER_C_FLAGS) -Gh -Zi -MD
C_DEFINES=$(C_DEFINES) -DPROFILE
!ENDIF

TARGETNAME=accwiz
TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_CRTDLL=1
USE_NATIVE_EH=1
TARGETLIBS=$(ICECAP_LIBS) \
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
           $(BASEDIR)\public\sdk\lib\*\shlwapi.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32p.lib \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib
   
NOT_LEAN_AND_MEAN=1

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;\
         $(BASEDIR)\private\windows\shell\shelldll;\
         $(BASEDIR)\private\windows\shell\inc;\
         $(BASEDIR)\private\windows\inc;\
         $(BASEDIR)\private\windows\inc16;\
         $(BASEDIR)\private\inc; \
         $(BASEDIR)\public\sdk\inc\mfc42;\


SOURCES= ..\Accwiz.cpp      \
         ..\CurSchme.cpp    \
         ..\DLGFONTS.CPP    \
         ..\LOOKDLG.CPP     \
         ..\LOOKPREV.CPP    \
         ..\precomp.CPP     \
         ..\Pgbase.cpp      \
         ..\Pgfinish.cpp    \
         ..\PGGENRIC.CPP    \
         ..\PGLOKPRV.CPP    \
         ..\PGMSEBUT.CPP    \
         ..\Pgmsecur.cpp    \
         ..\PGSVEDEF.CPP    \
         ..\Pgsvefil.cpp    \
         ..\PGTMEOUT.CPP    \
         ..\pgnWelCome.cpp  \
         ..\pgWelco2.cpp    \
         ..\pgWelcom.cpp    \
         ..\Pgwizopt.cpp    \
         ..\SCHEMES.CPP     \
         ..\Select.cpp      \
         ..\accwiz.rc

!if $(386)
UMENTRYABS      = WinMainCRTStartup
!else
UMENTRYABS      = WinMainCRTStartup
!endif


LINKLIBS=

UMTYPE=windows

PRECOMPILED_INCLUDE=..\pch.hxx
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
PRECOMPILED_CXX=1

