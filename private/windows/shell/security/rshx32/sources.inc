!IF 0

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=shell
MINORCOMP=security

TARGETNAME=rshx32
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=DllMain
DLLDEF=$O\rshx32.def

UMTYPE=windows

DELAYLOAD=ole32.dll;shlwapi.dll;netapi32.dll;mpr.dll;winspool.drv

LINKLIBS=\
    $(SECLIB_PATH)\*\common.lib     \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\comctl32.lib    \
    $(SDK_LIB_PATH)\comctlp.lib     \
    $(SDK_LIB_PATH)\shell32.lib     \
    $(SDK_LIB_PATH)\shell32p.lib    \
    $(SDK_LIB_PATH)\shlwapi.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\winspool.lib    \
    $(SDK_LIB_PATH)\netapi32.lib    \
    $(SDK_LIB_PATH)\mpr.lib         \
    $(SDK_LIB_PATH)\uuid.lib        \

SOURCES=\
     ..\rshx32.rc       \
     ..\rshx32.cpp      \
     ..\cstrings.cpp    \
     ..\si.cpp          \
     ..\ntfssi.cpp      \
     ..\printsi.cpp     \
     ..\util.cpp        \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\rshx32.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
