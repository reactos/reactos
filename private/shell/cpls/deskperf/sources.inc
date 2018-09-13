WIN32_WINNT_VERSION = 0x0500

CCSHELL_DIR     = $(BASEDIR)\private\shell
!include $(CCSHELL_DIR)\common.inc

UMTYPE     = windows
TARGETNAME = deskperf
TARGETPATH = obj
TARGETTYPE = DYNLINK
TARGETEXT  = dll
DLLENTRY   = DllMain
COFFBASE   = desk

NO_BROWSER_FILE = 1

INCLUDES=..;$(BASEDIR)\private\windows\inc;$(BASEDIR)\private\shell\inc;$(BASEDIR)\private\inc

SOURCES = ..\deskperf.rc   \
          ..\deskperf.cpp  \
          ..\propsext.cpp  \
          ..\deskcmmn.cpp 



TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib  \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
           $(BASEDIR)\public\sdk\lib\*\user32p.lib  \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
           $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib \
           $(BASEDIR)\public\sdk\lib\*\comdlg32.lib

!ifdef CCOVER
TARGETLIBS = $(TARGETLIBS) $(BASEDIR)\public\sdk\lib\*\msvcrt.lib
!endif

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_CRTDLL=1
!ENDIF

DLLDEF=$(O)\deskperf.def
