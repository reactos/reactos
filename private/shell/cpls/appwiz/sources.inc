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

!include $(CCSHELL_DIR)\cpls\appwiz\appwiz.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\cpls\appwiz\appwiz.inc

SELFREGNAME     = selfreg_appwiz.inf

NO_BROWSER_FILE = 1

# Don't include core Windows resource header
C_DEFINES       = $(C_DEFINES) -DNOWINRES

# These are used in makefile.def when building the idl files.

PASS0_HEADERDIR = ..
PASS0_SOURCEDIR = ..

TARGETNAME    = appwiz
TARGETPATH    = obj
TARGETPATHLIB = $(BASEDIR)\public\sdk\lib
TARGETTYPE    = DYNLINK
TARGETEXT     = cpl

# Note:  Do >not< link to ntdll.  Win95 has an extremely minimal
#        implementation of ntdll.

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
!ifndef DOWNLEVEL_PLATFORM
                  $(LIBRARY_PATH)\shlwapip.lib      \
!endif
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(LIBRARY_PATH)\comctlp.lib       \
!ifdef TARGET_WIN95
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib      \
                  $(BASEDIR)\private\lib\chicago\*\shell32.w95  \
!else
                  $(LIBRARY_PLATFORM_PATH)\*\kernl32p.lib     \
! if $(386)	
                  $(BASEDIR)\private\lib\*\shell32.nt4 \
! else	
                  $(SDK_LIB_PATH)\shell32p.lib \
! endif	
!endif
                  $(CCSHELL_DIR)\lib\$O\stock5.lib            \

DELAYLOAD = OLE32.DLL;OLEAUT32.DLL

# nt5api must be finished before this one
SYNCHRONIZE_DRAIN=1

!ifdef DOWNLEVEL_PLATFORM

!ifdef TARGET_WIN95
LINKLIBS        = ..\w5api\$O\apithk.lib ..\w95c\$O\srcc.lib
!else #TARGET_WIN95
LINKLIBS        = ..\nt5api\$O\apithk.lib ..\nt4c\$O\srcc.lib
!endif #TARGET_WIN95

!else #DOWNLEVEL_PLATFORM

LINKLIBS        = ..\nt5api\$O\apithk.lib ..\ntc\$O\srcc.lib


!endif #DOWNLEVEL_PLATFORM

LINKLIBS        = $(LINKLIBS) \
                  $(CCSHELL_DIR)\lib\$O\stocklib.lib

!if $(IA64) || $(AXP64)
USE_LIBCMT      = 1
DLLENTRY        = _DllMainCRTStartup
!else
USE_IERT        = 1
DLLENTRY        = DllMain
!endif

USE_STATIC_ATL  = 1

DLLDEF          = $(O)\appwiz.def
DLLBASE         = 0x71500000

# For RC to find shappmgr.TLB
INCLUDES        = $(SDK_LIB_PATH);$(INCLUDES)

# Include makefile.inc
NTTARGETFILES   =

# Pass zero stuff
NTTARGETFILE0   =

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\priv.h


# Maintain the sources list in the alphabetical order

SOURCES         = \
                  ..\adcctl.cpp    \
                  ..\appmgr.cpp    \
                  ..\appsize.cpp   \
                  ..\appwiz.rc     \
                  ..\awthunk.c     \
                  ..\datasrc.cpp   \
                  ..\darpub.cpp    \
                  ..\darenum.cpp   \
                  ..\darapp.cpp    \
                  ..\debug.cpp     \
                  ..\dlinst.cpp    \
                  ..\dll.cpp       \
                  ..\dlldatax.c    \
                  ..\event.cpp     \
                  ..\findapp.cpp   \
                  ..\instapp.cpp   \
                  ..\instenum.cpp  \
                  ..\mtxarray.cpp  \
                  ..\pubenum.cpp   \
                  ..\sccls.c       \
                  ..\setupenum.cpp \
                  ..\shlwapi.c     \
                  ..\slowfind.cpp  \
                  ..\stdafx.cpp    \
                  ..\tasks.cpp     \
                  ..\util.cpp      \
                  ..\worker.cpp


# Binplace these files too...
!if "$(BUILD_PRODUCT)" == "NT"
MISCFILES       = $(MISCFILES) \
                  ..\$(SELFREGNAME)
!endif
