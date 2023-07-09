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

# Build a comctlp.lib
BUILD_PRIVLIB   = 1

SELFREGNAME     = selfreg_comctl32.inf

!include $(CCSHELL_DIR)\comctl32\comctl32.inc

NO_BROWSER_FILE = 1

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\comctl32\comctl32.inc

NTPROFILEINPUT  = 1

TARGETNAME      = comctl32
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

TARGETPATHLIB   = $(LIBRARY_PLATFORM_PATH)

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib        \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib     \
                  $(LIBRARY_PLATFORM_PATH)\*\user32.lib       \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib              \
                  $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib     \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib             \
                  $(LIBRARY_PATH)\uuid.lib                    \
                  $(CCSHELL_DIR)\lib\$O\stock5.lib            \

!IF $(ALPHA) && !$(AXP64)
TARGETLIBS      = $(TARGETLIBS) \
                  $(LIBRARY_PLATFORM_PATH)\*\wx86ofl.lib
!ENDIF

!if $(IA64) || $(AXP64)
USE_LIBCMT      = 1
!else
USE_IERT        = 1
!endif

DLLDEF          = $(O)\comctl32.def

PRIVDEF         = $(O)\comctlp.def
PRIVDEFSRC      = ..\comctl32.src
PRIVLIB         = comctlp.lib

DLLENTRY        = LibMain

# Wait for srccpp.lib and apithk.lib to be built
SYNCHRONIZE_DRAIN=1

!if defined(TARGET_WIN95)
LINKLIBS        = ..\w95cpp\$O\srccpp.lib ..\w5api\$O\apithk.lib
!else
LINKLIBS        = ..\ntcpp\$O\srccpp.lib ..\nt5api\$O\apithk.lib
TARGETLIBS      = $(TARGETLIBS) \
                  $(LIBRARY_PATH)\user32p.lib
!endif

LINKLIBS        = $(LINKLIBS) \
                  $(CCSHELL_DIR)\lib\$(O)\stocklib.lib

SOURCES         = ..\animate.c   \
                  ..\comboex.c   \
                  ..\commctrl.rc \
                  ..\commctrl.c  \
                  ..\cstrings.c  \
                  ..\cutils.c    \
                  ..\da.c        \
                  ..\draglist.c  \
                  ..\flat_sb.c   \
                  ..\header.c    \
                  ..\hotkey.c    \
                  ..\image.c     \
                  ..\ipaddr.c    \
                  ..\listview.c  \
                  ..\lvicon.c    \
                  ..\lvlist.c    \
                  ..\lvrept.c    \
                  ..\lvsmall.c   \
                  ..\mem.c       \
                  ..\menuhelp.c  \
                  ..\monthcal.c  \
                  ..\mru.c       \
                  ..\notify.c    \
                  ..\os.c        \
                  ..\progress.c  \
                  ..\prsht.c     \
                  ..\prpage.c    \
                  ..\reader.c    \
                  ..\rebar.c     \
                  ..\rlefile.cpp \
                  ..\scdttime.c  \
                  ..\status.c    \
                  ..\strings.c   \
                  ..\subclass.c  \
                  ..\treeview.c  \
                  ..\tab.c       \
                  ..\tbcust.c    \
                  ..\thunk.c     \
                  ..\toolbar.c   \
                  ..\tooltips.c  \
                  ..\trackbar.c  \
                  ..\trackme.c   \
                  ..\tvmem.c     \
                  ..\tvpaint.c   \
                  ..\tvscroll.c  \
                  ..\updown.c    \
                  ..\mirror.c

PRECOMPILED_INCLUDE = ..\ctlspriv.h
PRECOMPILED_PCH = ctlspriv.pch
PRECOMPILED_OBJ = ctlspriv.obj

# Binplace these files too...
!if "$(BUILD_PRODUCT)" == "NT"
MISCFILES       = $(MISCFILES) \
                  $(SELFREGNAME)
!endif
