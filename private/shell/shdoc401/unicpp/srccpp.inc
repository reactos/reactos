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

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\unicpp\srccpp.inc

INDENTED_DIRECTIVES=1

MAJORCOMP       = shell
MINORCOMP       = unicpp

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!include $(CCSHELL_DIR)\shdoc401\shdoc401.inc

INCLUDES=$(CCSHELL_DIR)\shdoc401;$(INCLUDES)

NO_BROWSER_FILE = 1

TARGETNAME      = unicpp
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = \
                  adjust.cpp   \
                  admover.cpp  \
                  admoveri.cpp \
                  cnctnpt.cpp  \
                  cobjsafe.cpp \
                  cpymovto.cpp \
                  dback.cpp    \
                  dbackp.cpp   \
                  dcomp.cpp    \
                  dcompp.cpp   \
                  deskcls.cpp  \
                  deskhtml.cpp \
                  deskmovr.cpp \
                  deskstat.cpp \
                  dspsprt.cpp  \
                  dsubscri.cpp \
                  dvoc.cpp     \
                  dvutil.cpp   \
                  dutil.cpp    \
                  dvutil.cpp   \
                  editpat.cpp  \
                  expdsprt.cpp \
                  fsupgwiz.cpp \
                  startmnu.cpp \
                  libx.cpp     \
                  options.cpp  \
                  advanced.cpp \
                  fldoppsx.cpp \
                  mimeflt.cpp  \
                  pattern.cpp  \
                  sdflditm.cpp \
                  sdfldits.cpp \
                  sdfldits.cpp \
                  sdfolder.cpp \
                  sdlink.cpp   \
                  sdmain.cpp   \
                  sdwindow.cpp \
                  shfvocx.cpp  \
                  stdafx.cpp   \
                  stdenum.cpp  \
                  mruex.cpp    \
                  desktop.cpp  \
                  dde.cpp      \
                  sendto.cpp   \
                  cmdfile.cpp  \
                  utils.cpp    \

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = stdafx.h
PRECOMPILED_PCH = stdafx.pch
PRECOMPILED_OBJ = shdoc401.obj

