!INCLUDE ..\..\dsui.inc

MAJORCOMP=dsui
MINORCOMP=dsuiext

TARGETNAME=dsuiext
TARGETTYPE=DYNLINK
TARGETPATH=obj
TARGETPATHLIB=$(LIBRARY_PLATFORM_PATH)
TARGETEXT=dll

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=..\dsuiext.def

C_DEFINES=  $(C_DEFINES) -DSECURITY_WIN32

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(LIBRARY_PLATFORM_PATH)\*\comctl32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib    \
            $(BASEDIR)\public\sdk\lib\*\ntdsapi.lib   \
            $(BASEDIR)\public\sdk\lib\*\wininet.lib   \
            $(BASEDIR)\public\sdk\lib\*\urlmon.lib    \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib   \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib  \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib      \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib     \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib  \
            $(BASEDIR)\public\sdk\lib\*\adsiid.lib    \
            $(BASEDIR)\public\sdk\lib\*\activeds.lib  \
            $(BASEDIR)\public\sdk\lib\*\mpr.lib       \
            $(SHELL_LIBS)

#
# Win95 requires us to link to a different library to get the DsGetDcName API
# as this is not exported from netapi32.
#

!IF "$(PLATFORM)"=="WINNT"
TARGETLIBS= $(TARGETLIBS) $(BASEDIR)\public\sdk\lib\*\netapi32.lib 
!ELSE
TARGETLIBS= $(TARGETLIBS) $(LIBRARY_PLATFORM_PATH)\*\logonsrv.lib           
!ENDIF

#
# WinNT requires the security APIS, so lets link to that DLL
#

!IF "$(PLATFORM)"=="WINNT"
TARGETLIBS= $(TARGETLIBS) $(BASEDIR)\public\sdk\lib\*\secur32.lib
!ENDIF

SOURCES=    ..\dll.cpp         \
            ..\core.cpp        \
            ..\tab.cpp         \
            ..\cache.cpp       \
            ..\cacheapi.cpp    \
            ..\icon.cpp        \
            ..\domain.cpp      \
            ..\dispspec.cpp    \
            ..\browse.cpp      \
            ..\verbs.cpp       \
            ..\strings.cpp     \
            ..\ds.cpp          \
            ..\resource.rc
