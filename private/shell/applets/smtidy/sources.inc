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

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME      = smtidy
TARGETPATH      = obj
TARGETTYPE      = PROGRAM

TARGETLIBS      = $(LIBRARY_PATH)\gdi32.lib             \
                  $(LIBRARY_PATH)\user32.lib            \
                  $(LIBRARY_PATH)\advapi32.lib          \
                  $(LIBRARY_PATH)\shlwapi.lib           \
                  $(LIBRARY_PATH)\comctlp.lib           \
                  $(LIBRARY_PATH)\shell32p.lib          \
                  $(LIBRARY_PATH)\uuid.lib              \
                  $(LIBRARY_PATH)\ntdll.lib             \
                  $(CCSHELL_DIR)\lib\$(O)\stocklib.lib

USE_LIBCMT      = 1

!if $(386)
UMENTRYABS      = ModuleEntry@0
!else
UMENTRYABS      = ModuleEntry
!endif

SOURCES         = ..\smtidy.rc     \
                  ..\init.c        \
                  ..\smwiz.c       \
                  ..\util.c        \
                  ..\filter.c      \
                  ..\killactv.c    \
                  ..\finish.c      \
                  ..\lnktrack.c

PRECOMPILED_INCLUDE = ..\smtidy.h
