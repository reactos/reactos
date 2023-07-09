CCSHELL_DIR = ..\..

!include $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

CCOVER =1

INCLUDES = $(CCSHELL_DIR)\comdlg32;$(INCLUDES)

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\comdlg32\comdlg32.inc $(CCSHELL_DIR)\common.inc

TARGETNAME    = comdlg32

TARGETPATHLIB = $(LIBRARY_PLATFORM_PATH)
TARGETPATH    = obj
TARGETTYPE    = DYNLINK

TARGETLIBS = $(BASEDIR)\public\sdk\lib\*\shlwapip.lib     \
             $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\user32.lib        \
             $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib         \
             $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib       \
             $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\shell32.lib       \
             $(LIBRARY_PLATFORM_PATH)\*\winspool.lib      \
             $(LIBRARY_PATH)\uuid.lib                     \
             $(CCSHELL_DIR)\lib\$O\shguidp.lib            \
             $(CCSHELL_DIR)\lib\$O\stock5.lib             \
             $(CCSHELL_DIR)\lib\$O\stocklib.lib

# ccover needs to link to C run-time lib, so we need to add msvcrt.lib to the link list
!ifdef CCOVER
TARGETLIBS =  $(TARGETLIBS) $(BASEDIR)\public\sdk\lib\*\msvcrt.lib
!endif

!if defined(TARGET_WIN95)
LINKLIBS = ..\w95cpp\$O\srccpp.lib
!else
LINKLIBS = ..\ntcpp\$O\srccpp.lib
!endif

DLLDEF   = $(O)\comdlg32.def

DLLENTRY = LibMain

DELAYLOAD = winspool.drv

SOURCES = ..\comdlg32.rc  \
          ..\color.c      \
          ..\color2.c     \
          ..\data.c       \
          ..\dlgs.c       \
          ..\dllload.c    \
          ..\fileopen.c   \
          ..\find.c       \
          ..\font.c       \
          ..\init.c       \
          ..\parse.c      \
          ..\prnsetup.c   \
          ..\debug.c

# PRECOMPILED_INCLUDE = ..\priv.h

