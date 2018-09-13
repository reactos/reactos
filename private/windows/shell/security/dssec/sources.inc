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
    Created 22-Nov-96 by Jeff Saathoff (jeffreys)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

MAJORCOMP=  shell
MINORCOMP=  security

TARGETNAME= dssec
TARGETTYPE= DYNLINK
TARGETPATH= obj
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib

DLLENTRY=   DllMain
DLLDEF=     $O\dssec.def

LINKLIBS=\
    $(SECLIB_PATH)\*\common.lib \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib    \
    $(SDK_LIB_PATH)\comctlp.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\adsiid.lib      \
    $(SDK_LIB_PATH)\activeds.lib    \

SOURCES=\
    ..\extrts.mc    \
    ..\resource.rc  \
    ..\dll.cpp      \
    ..\cstrings.cpp \
    ..\security.cpp \
    ..\dssi.cpp     \
    ..\schema.cpp   \
    ..\exnc.cpp     \

MISCFILES=\
    ..\dssec.dat

PASS0_SOURCEDIR=..
PASS0_HEADERDIR=..

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
