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


MAJORCOMP=shell
MINORCOMP=mag_hook

#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
ICECAP_LIBS=$(BASEDIR)\public\sdk\lib\*\icap.lib
USER_C_FLAGS=$(USER_C_FLAGS) -Gh -Zi -MD
C_DEFINES=$(C_DEFINES) -DPROFILE -DUNICODE -D_UINCODE
!ENDIF

TARGETNAME=Mag_Hook
TARGETPATH=obj
TARGETTYPE=DYNLINK
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
		   $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
           $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
           $(BASEDIR)\public\sdk\lib\*\oleacc.lib   \
		   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib
   
NOT_LEAN_AND_MEAN=1

DLLENTRY=DllMain
# DLLENTRY=_DllMainCRTStartup

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=.;\
         $(BASEDIR)\public\sdk\inc; \
         $(BASEDIR)\public\sdk\inc\mfc42;


SOURCES= ..\mag_hook.cpp \
         ..\mag_hook.rc \
         ..\msgqueue.cpp

LINKLIBS=

UMTYPE=windows
UMRES=obj\*\main.res
DLLDEF=..\Mag_Hook.def
