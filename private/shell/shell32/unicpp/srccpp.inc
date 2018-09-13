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

!include $(CCSHELL_DIR)\shell32\shell32.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\shell32.inc

NO_BROWSER_FILE = 1

TARGETNAME      = unicpp
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = \
                  ..\adjust.cpp   \
                  ..\admoveri.cpp \
                  ..\advanced.cpp \
                  ..\cmdfile.cpp  \
                  ..\cnctnpt.cpp  \
                  ..\cobjsafe.cpp \
                  ..\cpymovto.cpp \
                  ..\dback.cpp    \
                  ..\dbackp.cpp   \
                  ..\dcomp.cpp    \
                  ..\dcompp.cpp   \
                  ..\dde.cpp      \
                  ..\deskcls.cpp  \
                  ..\deskhtml.cpp \
                  ..\deskmovr.cpp \
                  ..\deskstat.cpp \
                  ..\desktop.cpp  \
                  ..\dspsprt.cpp  \
                  ..\dsubscri.cpp \
                  ..\dvoc.cpp     \
                  ..\dvutil.cpp   \
                  ..\dutil.cpp    \
                  ..\dvutil.cpp   \
                  ..\editpat.cpp  \
                  ..\expdsprt.cpp \
                  ..\fldoppsx.cpp \
                  ..\libx.cpp     \
                  ..\mimeflt.cpp  \
                  ..\mruex.cpp    \
                  ..\options.cpp  \
                  ..\pattern.cpp  \
                  ..\propbag.cpp  \
                  ..\sdflditm.cpp \
                  ..\sdfldits.cpp \
                  ..\sdfldits.cpp \
                  ..\sdfolder.cpp \
                  ..\sdlink.cpp   \
                  ..\sdmain.cpp   \
                  ..\sdwindow.cpp \
                  ..\sendto.cpp   \
                  ..\shfvocx.cpp  \
                  ..\startmnu.cpp \
                  ..\stdenum.cpp  \
                  ..\utils.cpp    \


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj
PRECOMPILED_SOURCEFILE=..\stdafx.cpp
