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

SYNCHRONIZE_DRAIN=1

TARGETNAME=cscui
TARGETPATH=obj
TARGETPATHLIB=$(CSCUILIB_PATH)
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
DLLDEF=$O\cscui.def
USE_CRTDLL=1
UMTYPE=windows

DELAYLOAD=ole32.dll;mpr.dll

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib        \
    $(SDK_LIB_PATH)\user32.lib          \
    $(SDK_LIB_PATH)\gdi32.lib           \
    $(SDK_LIB_PATH)\advapi32.lib        \
    $(SDK_LIB_PATH)\ole32.lib           \
    $(SDK_LIB_PATH)\comctl32.lib        \
    $(SDK_LIB_PATH)\comctlp.lib         \
    $(SDK_LIB_PATH)\shell32.lib         \
    $(BASEDIR)\private\shell\lib\$(O)\shguidp.lib \
    $(SDK_LIB_PATH)\shell32p.lib        \
    $(SDK_LIB_PATH)\shlwapip.lib        \
    $(SDK_LIB_PATH)\cscdll.lib          \
    $(SDK_LIB_PATH)\mpr.lib             \
    $(SDK_LIB_PATH)\uuid.lib



LINKLIBS=\
    $(CSCUILIB_PATH)\*\viewer.lib       \
    $(CSCUILIB_PATH)\*\dll.lib          \

SOURCES=\
    ..\uuid.c          \
    ..\cscui.rc

