WIN32_WINNT_VERSION = 0x0500
WIN32_WIN95_VERSION = 0x0500


CCSHELL_DIR     = $(BASEDIR)\private\shell
!include $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc


TARGETPATH      = obj
TARGETTYPE      = DYNLINK
TARGETEXT       = cpl
DLLENTRY        = DllInitialize
DLLDEF          = $O\desk.def
COFFBASE        = desk

INCLUDES=$(INCLUDES);..\;$(BASEDIR)\private\inc;$(BASEDIR)\public\sdk\inc\hydra



SOURCES         = ..\desk.rc      \
                  ..\backdlg.c    \
                  ..\backprev.c   \
                  ..\color2.c     \
                  ..\colorpic.c   \
                  ..\custfont.c   \
                  ..\debug.c      \
                  ..\desk.c       \
                  ..\dragsize.c   \
                  ..\exe.c        \
                  ..\fake.c       \
                  ..\fixreg95.c   \
                  ..\init.c       \
                  ..\install2.c   \
                  ..\lookdlg.c    \
                  ..\lookprev.c   \
                  ..\pattern.c    \
                  ..\ssdlg.c      \
                  ..\general.cpp  \
                  ..\ntreg.cpp    \
                  ..\settings.cpp \
                  ..\ocpage.cpp   \
                  ..\multimon.cpp \
                  ..\deskcmmn.cpp


TARGETLIBS=$(LIBRARY_PATH)\user32p.lib  \
           $(LIBRARY_PATH)\gdi32.lib    \
           $(LIBRARY_PATH)\comctl32.lib \
           $(LIBRARY_PATH)\comctlp.lib  \
           $(LIBRARY_PATH)\comdlg32.lib \
           $(LIBRARY_PATH)\syssetup.lib \
           $(LIBRARY_PATH)\setupapi.lib \
           $(LIBRARY_PATH)\cfgmgr32.lib \
           $(LIBRARY_PATH)\shlwapi.lib  \
           $(LIBRARY_PATH)\shlwapip.lib \
           $(LIBRARY_PATH)\shell32p.lib \
           $(LIBRARY_PATH)\mpr.lib      \
           $(LIBRARY_PATH)\version.lib  \
           $(LIBRARY_PATH)\advapi32.lib \
           $(LIBRARY_PATH)\uuid.lib     \
           $(LIBRARY_PATH)\oleaut32.lib \
           $(CCSHELL_DIR)\lib\$O\shguidp.lib  \
           $(CCSHELL_DIR)\lib\$O\stock5.lib \
           $(CCSHELL_DIR)\lib\$O\stocklib.lib \
!if defined(TARGET_WIN95)
           $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib            \
           $(BASEDIR)\private\lib\chicago\*\shell32.w95
!else
           $(LIBRARY_PATH)\kernel32.lib
!endif


DELAYLOAD=COMDLG32.DLL;SYSSETUP.DLL;SETUPAPI.DLL;MPR.DLL

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
