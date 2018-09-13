!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

       $(BASEDIR)\public\sdk\lib\*\user32.lib \
       $(BASEDIR)\public\sdk\lib\*\gdi32.lib  \
       $(BASEDIR)\public\sdk\lib\*\kernel32.lib  \

!ENDIF

MAJORCOMP=shell
MINORCOMP=dataclen

PRECOMPILED_CXX=1

PRECOMPILED_INCLUDE=..\common.h
SELFREGNAME      = selfreg_dataclen.inf

CCSHELL_DIR = $(BASEDIR)\private\shell\

!INCLUDE $(CCSHELL_DIR)\COMMON.INC

INCLUDES=$(CCSHELL_DIR)\inc\win95;$(INCLUDES)

NO_BROWSER_FILE = 1

TARGETNAME=dataclen
TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(LIBRARY_PATH)\shlwapip.lib

USE_MSVCRT=1

SOURCES     = ..\dataclen.rc \
              ..\midebug.cpp \
              ..\dataclen.cpp \
              ..\compclen.cpp

##!if $(386)
#UMENTRYABS      = ModuleEntry@0
#!else
#UMENTRYABS      = ModuleEntry
#!endif

DLLDEF          = ..\dataclen.def

!ifndef CCOVER
DLLENTRY        = LibMain
!else
DLLENTRY        = DllMain
!endif

DLLBASE         = 0x10000000

NTTARGETFILES   =

!if "$(BUILD_PRODUCT)" == "NT"
MISCFILES       = $(MISCFILES) \
                  $(SELFREGNAME)
!endif

