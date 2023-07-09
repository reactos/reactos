!IF 0

Copyright (c) 1989-1999  Microsoft Corporation.

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:

!ENDIF

MAJORCOMP=shell
MINORCOMP=accesories

TARGETNAME=eudcedit
TARGETPATH=obj
TARGETTYPE=LIBRARY
TARGETLIBS=


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

INCLUDES=..;..\..\..\inc;..\..\..\..\inc;..\..\..\..\..\inc

C_DEFINES=-DWINVER=0x0500 -DBUILD_ON_WINNT -DNEWREG

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\ASSOCDLG.CPP \
        ..\BLINKDLG.CPP \
        ..\BMPOPE.CPP   \
        ..\CODE.CPP     \
        ..\CREATE.CPP   \
        ..\DATAIF.CPP   \
        ..\EDITWND.CPP  \
        ..\ETEN.CPP     \
        ..\EUDCDLG.CPP  \
        ..\EUDCEDIT.CPP \
        ..\EUDCLIST.CPP \
        ..\EUDCRANG.CPP \
        ..\FITCONIC.CPP \
        ..\GAGEDLG.CPP  \
        ..\GUIDEBAR.CPP \
        ..\IMEBLINK.C   \
        ..\IMELIST.C    \
        ..\IMPORT.CPP   \
        ..\IMPRTDLG.CPP \
        ..\MAINFRM.CPP  \
        ..\MAKEPOLY.CPP \
        ..\REFRDLG.CPP  \
        ..\REFRLIST.CPP \
        ..\REFRWND.CPP  \
        ..\REGISTRY.CPP \
        ..\ROTATDLG.CPP \
        ..\SMOOTH.CPP   \
        ..\TTFFILE.CPP  \
        ..\UTIL.CPP     \
        ..\VDATA.CPP    \
        ..\W31JBMP.CPP  \
        ..\W31JEUDC.CPP \
        ..\EUDCEDIT.RC

UMTYPE=windows
UMENTRY=winmain
UMAPPL=eudcedit
UMLIBS=obj\*\eudcedit.lib \
       obj\*\eudcedit.res \
       $(BASEDIR)\public\sdk\lib\*\comdlg32.lib  \
       $(BASEDIR)\public\sdk\lib\*\shell32.lib   \
       $(BASEDIR)\public\sdk\lib\*\gdi32.lib     \
       $(BASEDIR)\public\sdk\lib\*\gdi32p.lib    \
       $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib  \
       $(BASEDIR)\public\sdk\lib\*\imm32.lib
