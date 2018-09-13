#
# Cross-compiler macros for unique naming
#
MAJORCOMP       = shell
MINORCOMP       = srvwiz

# relative path to root of shell project
CCSHELL_DIR     = $(BASEDIR)\private\shell

WIN32_WINNT_VERSION=0x0500

#
# Include our common makefile
#

!include $(CCSHELL_DIR)\common.inc

#
# Common settings required by build.exe and makefile.def
#

#UMTYPE     = windows
TARGETNAME = srvwiz
TARGETPATH = obj
TARGETTYPE = DYNLINK
DLLENTRY   = DllMain
TARGETEXT  = cpl

TARGETLIBS = \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\shell32p.lib      \
    $(SDK_LIB_PATH)\shlwapi.lib       \
    $(SDK_LIB_PATH)\shlwapip.lib      \
    $(CCSHELL_DIR)\lib\$O\stocklib.lib

USE_CRTDLL=1

INCLUDES=$(CCSHELL_DIR)\cpls\srvwiz5;$(INCLUDES)
 
C_DEFINES=$(C_DEFINES) /DSERVER=$(SERVER)

DLLDEF   = ..\srvwiz.def

#
# List of sources
#

SOURCES = \
    ..\srvwiz.cpp \
    ..\srvwiz.rc

