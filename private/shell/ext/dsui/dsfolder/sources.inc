!INCLUDE ..\..\dsui.inc

MAJORCOMP=dsui
MINORCOMP=dsfolder

TARGETNAME=dsfolder
TARGETTYPE=DYNLINK
TARGETPATH=obj
TARGETPATHLIB=$(LIBRARY_PLATFORM_PATH)
TARGETEXT=dll

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=..\dsfolder.def

INCLUDES=   $(INCLUDES)

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(LIBRARY_PLATFORM_PATH)\*\comctl32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib    \
            $(LIBRARY_PLATFORM_PATH)\*\comdlg32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\dsuiext.lib    \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib   \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib  \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib     \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib  \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib      \
            $(BASEDIR)\public\sdk\lib\*\activeds.lib  \
            $(BASEDIR)\public\sdk\lib\*\adsiid.lib    \
            $(BASEDIR)\public\sdk\lib\*\wininet.lib   \
            $(BASEDIR)\public\sdk\lib\*\netapi32.lib  \
            $(SHELL_LIBS)

SOURCES=    ..\cstrings.cpp   \
            ..\dataobj.cpp    \
            ..\dll.cpp        \
            ..\enum.cpp       \
            ..\folder.cpp     \
            ..\idlist.cpp     \
            ..\ui.cpp         \
            ..\resource.rc    \
