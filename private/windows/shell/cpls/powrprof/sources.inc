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

!include ..\powrprof.inc

SOURCES_USED    = $(SOURCES_USED) ..\powrprof.inc

TARGETNAME      = powrprof
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
DLLENTRY        = DllInitialize

TARGETPATHLIB   = $(LIBRARY_PLATFORM_PATH)

TARGETLIBS      = $(LIBRARY_PATH)\setupapi.lib                  \
                  $(LIBRARY_PLATFORM_PATH)\*\user32.lib         \
                  $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib          \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib       \
                  $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib        \
                  $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib       

USE_LIBCMT      = 1

DLLDEF          = ..\powrprof.def

UMENTRYABS      = ModuleEntry

SOURCES         = ..\powrprof.c      \
                  ..\reghelp.c       \
                  ..\merge.c         \
                  ..\debug.c         \
                  ..\powrprof.rc

