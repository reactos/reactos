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

!ENDIF

!include ..\powercfg.inc


SOURCES_USED    = $(SOURCES_USED) ..\powercfg.inc

TARGETNAME      = powercfg
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
DLLENTRY        = DllInitialize
TARGETEXT       = cpl

TARGETPATHLIB   = $(LIBRARY_PLATFORM_PATH)

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\user32.lib         \
                  $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib          \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib        \
                  $(LIBRARY_PLATFORM_PATH)\*\comdlg32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\batmeter.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\powrprof.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\setupapi.lib       \
!ifndef TARGET_WIN95
                  $(LIBRARY_PLATFORM_PATH)\*\oleaut32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\uuid.lib       \
!endif
                  ..\ups\lib\$(O)\ups.lib

USE_MSVCRT      = 1

DLLDEF          = $O\powercfg.def


SOURCES         = ..\powercfg.c      \
                  ..\alarm.c         \
                  ..\batmtrcf.c      \
                  ..\getset.c        \
                  ..\hibernat.c      \
                  ..\prshthlp.c      \
                  ..\pwrschem.c      \
                  ..\pwrswtch.c      \
                  ..\ntapm.c         \
                  ..\powercfg.rc
