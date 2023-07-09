!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

History:
    Created 22-Sep-97 by Jeff Saathoff (jeffreys)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=csc
MINORCOMP=cscui

TARGETNAME=dll
TARGETPATH=$(CSCUILIB_PATH)
TARGETTYPE=LIBRARY
TARGETLIBS=
LINKLIBS=

# Remove this line to disable hooks
#C_DEFINES = $(C_DEFINES) -DCSCUI_HOOKS

SOURCES=\
    ..\dll.cpp         \
    ..\util.cpp        \
    ..\filelist.cpp    \
    ..\msgbox.cpp      \
    ..\shellex.cpp     \
    ..\update.cpp      \
    ..\volclean.cpp    \
    ..\cscentry.cpp    \
    ..\cscst.cpp       \
    ..\strings.cpp     \
    ..\debug.cpp       \
    ..\atlcvt.cpp      \
    ..\admin.cpp       \
    ..\osver.cpp       \
    ..\folder.cpp      \
    ..\enum.cpp        \
    ..\items.cpp       \
    ..\security.cpp    \
    ..\idldata.cpp     \
    ..\idlhelp.cpp     \
    ..\fmtetc.cpp      \
    ..\firstpin.cpp    \
    ..\statdlg.cpp     \
    ..\uihooks.cpp     \
    ..\fopendlg.cpp    \
    ..\eventlog.cpp    \
    ..\sharecnx.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
