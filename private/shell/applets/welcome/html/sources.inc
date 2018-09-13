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

CCSHELL_DIR     = ..\..\..\..

#
# Cross-compiler macros for unique naming
#

MAJORCOMP       = shell
MINORCOMP       = welcome

#
# Defines for common.inc
#

# needed to get around stupid IE Build crap:
USE_NT_PRODUCT_VER = 1
WIN32_WINNT_VERSION=0x0500

#
# Include our common makefile
#

!include $(CCSHELL_DIR)\common.inc

#
# Common settings required by build.exe and makefile.def
#

TARGETNAME = welcome
TARGETPATH = obj
TARGETTYPE = PROGRAM
TARGETLIBS = \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\shdocvw.lib       \
    $(SDK_LIB_PATH)\shell32p.lib      \
    $(SDK_LIB_PATH)\shlwapi.lib       \
    $(SDK_LIB_PATH)\libcmt.lib      \


INCLUDES = $(SDK_INC_PATH)\atl21;$(INCLUDES)
 
UMENTRYABS=main

C_DEFINES=$(C_DEFINES) /DSERVER=$(SERVER)

#
# List of sources
#

SOURCES = \
    ..\container.cpp \
    ..\eventsink.cpp \
    ..\inidata.cpp \
    ..\resource.rc \
    ..\webapp.cpp \

MISCFILES = \
    welcome.ini \
    ..\res\flag.gif \
    ..\res\flagblk.gif

#
# Tempory hack to disable DA code until problems are resolved
# By defining USEDA before building the DA stuff can be turned
# back on for testing
#
!IF "$(USEDA)" == "1"
C_DEFINES=$(C_DEFINES) /DUSEDA
!ENDIF

#
# For test purposes you can have welcome navigate to about:blank
# instead of it's normal HTML page.  This helps test working set
# issues concerning the size of the page itself.  To do this
# define TEST_WITH_ABOUT_BLANK before compiling welcome.
#
!IF "$(TEST_WITH_ABOUT_BLANK)" == "1"
C_DEFINES=$(C_DEFINES) /DTEST_WITH_ABOUT_BLANK
!ENDIF

#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) -DPROFILE
USER_C_FLAGS=$(USER_C_FLAGS) /Gh /Zi
TARGETLIBS=$(TARGETLIBS) $(SDK_LIB_PATH)\icap.lib
!ENDIF

