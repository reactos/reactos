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

!include $(CCSHELL_DIR)\comctl32\comctl32.inc

NO_BROWSER_FILE = 1

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\comctl32\comctl32.inc

PRECOMPILED_CXX = 1

TARGETNAME      = srccpp
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = ..\olestuff.cpp \
                  ..\ddproxy.cpp \
                  ..\dlgcvt.cpp \
                  ..\pager.cpp \
                  ..\dlgcset.cpp \
                  ..\selrange.cpp \
                  ..\fontlink.cpp \
                  ..\unicwrap.cpp
            

PRECOMPILED_INCLUDE = ..\ctlspriv.h
PRECOMPILED_PCH = ctlspriv.pch
PRECOMPILED_OBJ = ctlspriv.obj

