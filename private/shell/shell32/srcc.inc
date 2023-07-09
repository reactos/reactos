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

# Build a shell32p.lib
BUILD_PRIVLIB   = 1
SELFREGNAME     = selfreg_shell32.inf

CCSHELL_DIR     = $(BASEDIR)\private\shell

!include $(CCSHELL_DIR)\shell32\shell32.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\shell32.inc

NO_BROWSER_FILE = 1

TARGETNAME      = shell32
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

TARGETPATHLIB   = $(LIBRARY_PLATFORM_PATH)

#----- THIS COMMENT BLOCK IS OBSOLETE BUT CONTAINS USEFUL INFO SO I'M
#      KEEPING IT
#
#   shlwapip is weird.
#
#   We want to put shlwapip.lib into the LINKLIBS because we have to
#   ensure that it is built as a prerequisite to ourselves.
#
#   However, LINKLIBS are linked before TARGETLIBS, which is a problem
#   because shlwapip contains overrides for standard Win32 functions
#   which we do not want to use.  (The overrides come from the Trident
#   people, who apparently rely on them.)
#
#   The secret is IMPLIB_OBJS.  We list shlwapip.lib there to indicate
#   that we need it built in order to link, and also list it in TARGETLIBS
#   to indicate the link order.
#

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib          \
                  $(LIBRARY_PLATFORM_PATH)\*\user32.lib         \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib       \
                  $(LIBRARY_PATH)\adsiid.lib         \
                  $(LIBRARY_PATH)\shlwapip.lib                  \
                  $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib        \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib              \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib             \
                  $(LIBRARY_PATH)\uuid.lib                      \
                  $(CCSHELL_DIR)\lib\$O\stock5.lib


!if defined(TARGET_WIN95)
TARGETLIBS      = $(TARGETLIBS) \
                  $(CCSHELL_DIR)\lib\$O\badappsa.lib    \

LINKLIBS        = ..\w95cpp\$O\srccpp.lib        \
                  ..\w95c\$O\srcc.lib            \
                  ..\unicpp\w95cpp\$O\unicpp.lib \
                  ..\w5api\$O\apithk.lib         \

!else
TARGETLIBS      = $(TARGETLIBS) \
                  $(LIBRARY_PATH)\ntdll.lib     \
                  $(LIBRARY_PATH)\rpcrt4.lib    \
                  $(LIBRARY_PATH)\user32p.lib   \
                  $(CCSHELL_DIR)\lib\$O\badappsu.lib    \

LINKLIBS        =  \
                  ..\unicode\$O\unicode.lib     \
                  ..\ntcpp\$O\srccpp.lib        \
                  ..\ntc\$O\srcc.lib            \
                  ..\unicpp\ntcpp\$O\unicpp.lib \
                  ..\nt5api\$O\apithk.lib       \
                  $(LIBRARY_PATH)\tracking.lib  \


DLLORDER        = $(BASEDIR)\private\prf\shell32.prf

! if "$(DELAYLOAD)" != ""
DELAYLOAD       = $(DELAYLOAD);rpcrt4.dll
! else
DELAYLOAD       = rpcrt4.dll
! endif

!endif # !if defined(TARGET_WIN95) ... !else

IMPLIB_OBJS     = $(IMPLIB_OBJS)                \
                  $(LIBRARY_PATH)\shlwapip.lib

LINKLIBS        = $(LINKLIBS) \
                  $(CCSHELL_DIR)\lib\$O\stocklib.lib

DLLDEF          = $O\shell32.def

PRIVDEFSRC      = ..\shell32.src
PRIVDEF         = $(O)\shell32p.def
PRIVLIB         = shell32p.lib

!if $(IA64) || $(AXP64)
USE_LIBCMT      = 1
DLLENTRY        = _DllMainCRTStartup
!else
USE_IERT        = 1
# ccover needs c run time, so we need to have entry point called DllMain
!ifdef CCOVER
DLLENTRY        = DllMain
!else
DLLENTRY        = DllEntry
!endif

!endif



SYNCHRONIZE_DRAIN=1

NTTARGETFILES   =

# We want to include some GUIDs in the shell32 import library
DLLLIBOBJECTS   = $(CCSHELL_DIR)\lib\$O\shguid.lib

SOURCES         = \
                  ..\shell32.rc  \
                  ..\about.c     \
                  ..\bitbuck.c   \
                  ..\bitbcksf.c  \
                  ..\bookmk.c    \
                  ..\cabstate.c  \
                  ..\clsobj.c    \
                  ..\commobj.c   \
                  ..\control1.c  \
                  ..\copy.c      \
                  ..\copyhook.c  \
                  ..\cplobj.c    \
                  ..\cstrings.c  \
                  ..\debug.c     \
                  ..\defclsf.c   \
                  ..\defext.c    \
                  ..\defxicon.c  \
                  ..\dllload.c   \
                  ..\docfind2.c  \
                  ..\drawpie.c   \
                  ..\drvxc.c     \
                  ..\exec.c      \
                  ..\extract.c   \
                  ..\expenv.c    \
                  ..\fileicon.c  \
                  ..\filetbl.c   \
                  ..\fsassoc.c   \
                  ..\fsnotify.c  \
                  ..\fstreex.c   \
                  ..\fsdrop.c    \
                  ..\brfcase.c   \
                  ..\futil.c     \
                  ..\grepfind.c  \
                  ..\grepqmt.c   \
                  ..\grepsrch.c  \
                  ..\grepstrs.c  \
                  ..\hash.c      \
                  ..\help.c      \
                  ..\idldata.c   \
                  ..\idldrop.c   \
                  ..\idlist.c    \
                  ..\idmk.c      \
                  ..\init.c      \
                  ..\link.c      \
                  ..\lnktrack.c  \
                  ..\lvutil.c    \
                  ..\malloc.c    \
                  ..\mrsw.c      \
                  ..\msgbox.c    \
                  ..\mulprsht.c  \
                  ..\multimon.c  \
                  ..\netapi.c    \
                  ..\netfind.c   \
                  ..\netviewx.c  \
                  ..\ole2def.c   \
                  ..\ole2dup.c   \
                  ..\ole2dup2.c  \
                  ..\os.c        \
                  ..\path.c      \
                  ..\folder.c    \
                  ..\fldsets.c   \
                  ..\pickicon.c  \
                  ..\prcache.c   \
                  ..\printer.c   \
                  ..\printer1.c  \
                  ..\printobj.c  \
                  ..\prqwnd.c    \
                  ..\prtprop.c   \
                  ..\psxa.c      \
                  ..\rdrag.c     \
                  ..\reglist.c   \
                  ..\restart.c   \
                  ..\rundll32.c  \
                  ..\securent.c  \
                  ..\shared.c    \
                  ..\shlexec.c   \
                  ..\shlexec2.c  \
                  ..\shlnot.c    \
                  ..\shlobjs.c   \
                  ..\shprsht.c   \
                  ..\shtdndlg.c  \
                  ..\stream.c    \
                  ..\undo.c      \
                  ..\version.c   \
                  ..\viewcomm.c  \
                  ..\ynlist.c

PRECOMPILED_INCLUDE = ..\shellprv.h

# Binplace these files too...
!if "$(BUILD_PRODUCT)" == "NT"
MISCFILES       = $(MISCFILES) \
                  $(SELFREGNAME)
!endif
