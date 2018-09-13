!INCLUDE ..\..\dsui.inc

MAJORCOMP=dsui
MINORCOMP=dsquery

TARGETNAME=dsquery
TARGETTYPE=DYNLINK
TARGETPATH=obj
TARGETPATHLIB=$(LIBRARY_PLATFORM_PATH)
TARGETEXT=dll

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=$(O)\dsquery.def

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(LIBRARY_PLATFORM_PATH)\*\comctl32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib    \
            $(LIBRARY_PLATFORM_PATH)\*\comdlg32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\dsuiext.lib    \
            $(LIBRARY_PLATFORM_PATH)\*\dsfolder.lib   \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib   \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib  \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib     \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib  \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib      \
            $(BASEDIR)\public\sdk\lib\*\activeds.lib  \
            $(BASEDIR)\public\sdk\lib\*\adsiid.lib    \
            $(BASEDIR)\public\sdk\lib\*\winspool.lib  \
            $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib  \
            $(BASEDIR)\public\sdk\lib\*\ntdsapi.lib   \
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

SOURCES=    ..\dll.cpp        \
            ..\cstrings.cpp   \
            ..\query.cpp      \
            ..\queryhlp.cpp   \
            ..\thread.cpp     \
            ..\scopes.cpp     \
            ..\find.cpp       \
            ..\params.cpp     \
            ..\propbag.cpp    \
            ..\io.cpp         \
            ..\qf_user.cpp    \
            ..\qf_print.cpp   \
            ..\qf_comp.cpp    \
            ..\qf_ldap.cpp    \
            ..\qf_share.cpp   \
            ..\qf_ou.cpp      \
            ..\qf_pwell.cpp   \
            ..\qf_dmctl.cpp   \
            ..\qf_frsmb.cpp   \
            ..\dsquery.rc
