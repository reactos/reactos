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
    Created 25-Sep-96 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=shell
MINORCOMP=security

TARGETNAME=aclui
TARGETPATH=obj
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib
TARGETTYPE=DYNLINK

DLLENTRY=DllMain
DLLDEF=$O\aclui.def
USE_NTDLL=1

UMTYPE=windows

DELAYLOAD=netapi32.dll;secur32.dll

LINKLIBS=\
    $(SECLIB_PATH)\*\common.lib     \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\gdi32.lib       \
    $(SDK_LIB_PATH)\shlwapi.lib     \
    $(SDK_LIB_PATH)\comctl32.lib    \
    $(SDK_LIB_PATH)\comctlp.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\netapi32.lib    \
    $(SDK_LIB_PATH)\adsiid.lib      \
    $(SDK_LIB_PATH)\secur32.lib     \

SOURCES=\
    ..\aclui.rc     \
    ..\uuid.c       \
    ..\dll.cpp      \
    ..\ace.cpp      \
    ..\acelist.cpp  \
    ..\owner.cpp    \
    ..\aceedit.cpp  \
    ..\chklist.cpp  \
    ..\misc.cpp     \
    ..\pagebase.cpp \
    ..\perm.cpp     \
    ..\permset.cpp  \
    ..\cstrings.cpp \
    ..\sidcache.cpp \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\aclpriv.h
PRECOMPILED_PCH=aclpriv.pch
PRECOMPILED_OBJ=aclpriv.obj
