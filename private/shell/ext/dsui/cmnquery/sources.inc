!INCLUDE ..\..\dsui.inc

MAJORCOMP=dsui
MINORCOMP=cmnquery

TARGETNAME=cmnquery
TARGETTYPE=DYNLINK
TARGETPATH=obj
TARGETEXT=dll

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=..\cmnquery.def

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\comctl32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib    \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib   \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib  \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib     \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib  \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib

SOURCES=    ..\dll.cpp      \
            ..\query.cpp    \
            ..\thunk.cpp    \
            ..\resource.rc

