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

CCSHELL_DIR     = $(BASEDIR)\private\shell

# Build a browseuip.lib
BUILD_PRIVLIB   = 1
SELFREGNAME     = ..\selfreg_browseui.inf

NO_BROWSER_FILE = 1

!include $(CCSHELL_DIR)\browseui\browseui.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\browseui\browseui.inc

#
# ! !   R E A D   M E   ! !
#
# This sources.inc file is shared to build two versions of BROWSEUI.DLL.
# One version is the primary dll, the other is the alternate dll.  Exactly
# what this means depends on which team you're working on.
#
# The IE team needs to do some work under #ifdef IE_BETA2 for some select
# files (listed in the SOURCES= list below).  The shell team needs to
# deliver code to the NT team that does not contain these changes.  Likewise,
# the IE build lab will be building the IE_BETA2 code, while the NT build lab
# will not.
#
# The BUILD_PRODUCT environment variable will determine which version
# is the primary and which version is the alternate DLL.
#
#   "IE"    - browseui\dll builds with the IE_BETA2 code ON
#             browseui\dllalt builds with the IE_BETA2 code off
#
#   "NT"    - browseui\dll builds with the IE_BETA2 code OFF
#             browseui\dllalt builds with the IE_BETA2 code on
#
# We allow for both versions to be built by both teams to catch potential
# build breaks.  Please be diligent and allow both dlls to be built.  Keeping
# the SOURCES= list below short will maintain a quicker build-time.
#

# (davemi) as of 8/18/98 there is no need to build alt
#!ifdef ALT_DLL
#
## This alternate BROWSEUI.DLL will be propped to \nt\drop\*\alt, so it
## does not stomp on the standard browseui.dll.
#
#ALT_PROJECT_TARGET = alt
#
## NT alternate builds will get the IE_BETA2 turned on
#!if "$(BUILD_PRODUCT)" == "NT"
#C_DEFINES   = $(C_DEFINES) -DIE_BETA2
#!endif
#
#!else
#
## IE primary builds will get the IE_BETA2 turned on
#!if "$(BUILD_PRODUCT)" == "IE"
#C_DEFINES   = $(C_DEFINES) -DIE_BETA2
#!endif
#
#!endif


# For RC to find exdisp.TLB
INCLUDES        = $(SDK_LIB_PATH);$(INCLUDES);$O;

# Generate the selfreg file before compiling the RC file
NTTARGETFILE0  = $(SELFREGNAME)


# !!!!!!!!!!!!!!!!!
#
# READ THIS!!!  Do not add -DFEATURE_* defines here. Add it to browseui.inc.
#
# !!!!!!!!!!!!!!!!!

TARGETNAME      = browseui
TARGETPATH      = obj
TARGETPATHLIB   = $(BASEDIR)\public\sdk\lib
TARGETTYPE      = DYNLINK

# NOTE!! We deliberately link to an NT4 compatible shell32.lib
#        so browseui can be loaded in the browser-only installations.
#        We use the NT version because it contains lego info.
#        The debug version of browseui links to the Win95 version
#        of shell32.lib simply so we can verify that we load in the
#        browser-only installations on Win95.
#
# Note:  Do >not< link to ntdll.  Win95 has an extremely minimal
#        implementation of ntdll.
#
# Note:  Do not link to wininet here.  It is delay-loaded.  If you need
#        a new function from wininet, put it in dllload.c.
#

!if 0
                  $(CCSHELL_DIR)\evtmon\inc\msoemad.lib
!endif
TARGETLIBS      = $(SDK_LIB_PATH)\kernel32.lib      \
                  $(LIBRARY_PATH)\shlwapip.lib      \
                  $(LIBRARY_PATH)\shdocvwp.lib      \
                  $(LIBRARY_PATH)\shdocvw.lib       \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\htmlhelp.lib      \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(LIBRARY_PATH)\comctl32.lib      \
                  $(LIBRARY_PATH)\comctlp.lib       \
!if $(386)
! if !$(FREEBUILD)
                  $(BASEDIR)\private\lib\chicago\*\shell32.w95  \
! else
                  $(BASEDIR)\private\lib\*\shell32.nt4  \
! endif
!else
                  $(SDK_LIB_PATH)\shell32p.lib        \
!endif
                  $(CCSHELL_DIR)\lib\$O\stock5.lib


!if $(IA64) || $(AXP64)
USE_LIBCMT      = 1
!else
USE_IERT        = 1
!endif

# srcc must be finished before this one
SYNCHRONIZE_DRAIN=1

LINKLIBS        = ..\srcc\$O\srcc.lib       \
                  ..\srccpp\$O\srccpp.lib   \
                  ..\nt5api\$O\apithk.lib   \
                  ..\emclient\$O\emclient.lib \
                  $(CCSHELL_DIR)\lib\$O\stocklib.lib \

DLLDEF          = $(O)\browseui.def
DLLBASE         = 0x71500000
!ifdef CCOVER
DLLENTRY        = DllMain
!else
DLLENTRY        = LibMain
!endif
DLLORDER        = $(BASEDIR)\private\prf\browseui.prf

PRIVDEF         = $(O)\browseuip.def
PRIVDEFSRC      = ..\browseui.src
PRIVLIB         = browseuip.lib


# (scotth) 6/17/98: in order to accomodate both IE and shell dev teams
#                   during the differing beta cycles, files in the list
#                   below get built twice: once with IE_BETA2 defined,
#                   and another time with IE_BETA2 undefined.  See the
#                   big README note above.

# Maintain the sources list in the alphabetical order
SOURCES         = ..\browseui.rc

PRECOMPILED_CXX   = 1
PRECOMPILED_INCLUDE = ..\priv.h

# Binplace these files too...
!if "$(BUILD_PRODUCT)" == "NT"
MISCFILES       = $(MISCFILES) \
                  $(SELFREGNAME)
!endif
