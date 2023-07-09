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
MINORCOMP=cleanmgr

PRECOMPILED_CXX=1

PRECOMPILED_INCLUDE=..\common.h

CCSHELL_DIR = ..\..\..\..
!INCLUDE $(CCSHELL_DIR)\COMMON.INC

INCLUDES=$(CCSHELL_DIR)\inc\win95;$(INCLUDES)

NO_BROWSER_FILE = 1

USE_MSVCRT=1

TARGETNAME=cleanmgr
TARGETPATH=obj
TARGETTYPE=PROGRAM

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(LIBRARY_PATH)\shlwapip.lib      \
                  $(LIBRARY_PATH)\comctlp.lib       \
                  $(LIBRARY_PATH)\shell32p.lib      \
                  $(CCSHELL_DIR)\lib\$O\stocklib.lib

SOURCES     = ..\cleanmgr.rc \
              ..\cmstrings.mc \
              ..\cleanmgr.cpp \
              ..\dmgrdlg.cpp \
              ..\dmgrinfo.cpp \
              ..\diskutil.cpp \
              ..\msprintf.cpp \
              ..\callback.cpp \
              ..\seldrive.cpp \
              ..\midebug.cpp

#!if $(386)
#UMENTRYABS      = ModuleEntry@0
#!else
#UMENTRYABS      = ModuleEntry
#!endif

