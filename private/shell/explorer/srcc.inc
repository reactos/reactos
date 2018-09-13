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

!include $(CCSHELL_DIR)\explorer\explorer.inc

NO_BROWSER_FILE = 1

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\explorer\explorer.inc

TARGETNAME      = explorer
TARGETPATH      = obj
TARGETTYPE      = PROGRAM
DLLORDER        = $(BASEDIR)\private\prf\explorer.prf
UMBASE          = 0x00400000

TARGETPATHLIB   = $(LIBRARY_PLATFORM_PATH)

!if !defined(TARGET_WIN95)
NTDLL_LIBS      = $(LIBRARY_PATH)\ntdll.lib         \
                  $(LIBRARY_PATH)\user32p.lib
!else
NTDLL_LIBS      = $(LIBRARY_PLATFORM_PATH)\*\user32.lib
!endif

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib          \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib       \
                  $(NTDLL_LIBS)                                 \
                  $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib       \
!if !defined(TARGET_WIN95)
                  $(LIBRARY_PLATFORM_PATH)\*\mpr.lib            \
!endif
                  $(LIBRARY_PATH)\shlwapip.lib                  \
                  $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib        \
                  $(LIBRARY_PATH)\uuid.lib                      \
                  $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib       \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib            \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib           \
                  $(LIBRARY_PATH)\ole32.lib                     \
                  $(SDK_LIB_PATH)\oleaut32.lib                  \
                  $(CCSHELL_DIR)\lib\$O\stock5.lib              \
                  $(CCSHELL_DIR)\lib\$(O)\stocklib.lib

# ccover wants to use c run time, so we have to add it to the libs

!ifdef CCOVER
TARGETLIBGS = $(TARGETLIBS) $(LIBRARY_PLATFORM_PATH)\*\libcmt.lib
!endif

USE_LIBCMT      = 1

!if defined(TARGET_WIN95)
LINKLIBS        = ..\w5api\$O\apithk.lib
!else
LINKLIBS        = ..\nt5api\$O\apithk.lib ..\ntc\$O\srcc.lib
!endif


# bugbug: scousens.
#         @0 needed because ModuleEntry is _stdcall.
#         On mips _stdcall is mapped to nothing

# when building CCOVER build, we use main() as entry point, 
# thus the following would not be needed.
!ifndef CCOVER
!if $(386)
UMENTRYABS      = ModuleEntry@0
!else
UMENTRYABS      = ModuleEntry
!endif
!endif


SOURCES         = ..\explorer.rc \
                  ..\bandsite.cpp \
                  ..\cfgstart.c \
                  ..\debug.cpp  \
                  ..\deskconf.c \
                  ..\initcab.cpp \
                  ..\libx.cpp    \
                  ..\muext.c    \
                  ..\multimon.c \
                  ..\startmnu.cpp \
                  ..\task.c     \
                  ..\taskband.cpp \
                  ..\taskbar.cpp \
                  ..\tray.c     \
                  ..\trayclok.c \
                  ..\traynot.c  \
                  ..\dllload.c

PRECOMPILED_INCLUDE = ..\cabinet.h

