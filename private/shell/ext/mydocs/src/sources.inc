!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include $(CCSHELL_DIR)\ext\mydocs\src\mydocs.inc

NO_BROWSER_FILE = 1

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\ext\mydocs\src\mydocs.inc

MAJORCOMP=  shell
MINORCOMP=  mydocs

TARGETNAME= mydocs
TARGETTYPE= DYNLINK
TARGETPATH= obj
TARGETEXT=  dll

DELAYLOAD=OLEAUT32.DLL
DLLDEF=..\mydocs.def
DLLENTRY=   DllMain
DLLBASE= 0x77000000

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF

USE_IERT=1

TARGETLIBS= $(CCSHELL_DIR)\ext\mydocs\lib\$(MYLIBDIR)\*\common.lib \
            $(CCSHELL_DIR)\lib\$(O)\stocklib.lib        \
            $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
            $(BASEDIR)\public\sdk\lib\*\user32.lib      \
            $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
            $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
            $(BASEDIR)\public\sdk\lib\*\shell32p.lib    \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib     \
            $(CCSHELL_DIR)\lib\$(O)\shguidp.lib         \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib    \
            $(BASEDIR)\public\sdk\lib\*\shdocvw.lib     \
            $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
            $(BASEDIR)\public\sdk\lib\*\comctl32.lib    \
            $(BASEDIR)\public\sdk\lib\*\comctlp.lib     \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib


INCLUDES=   ..\;   \
            $(CCSHELL_DIR)\shell32; \
            $(CCSHELL_DIR)\inc; \
            $(BASEDIR)\private\windows\inc; \
            $(BASEDIR)\private\windows\inc16; \
            $(CCSHELL_DIR)\ext\mydocs\inc; \
            $(CCSHELL_DIR)\ext\mydocs\src

SOURCES=    ..\resource.rc     \
            ..\cstrings.cpp    \
            ..\copyhook.cpp    \
            ..\dll.cpp         \
            ..\dataobj.cpp     \
            ..\enum.cpp        \
            ..\factory.cpp     \
            ..\folder.cpp      \
            ..\idlist.cpp      \
            ..\icon.cpp        \
            ..\prop.cpp        \
            ..\util.cpp        \
            ..\viewcb.cpp

C_DEFINES = $(C_DEFINES)

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
