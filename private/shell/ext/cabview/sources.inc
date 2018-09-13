!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Created 25-May-96 by Dave Plummer (davepl)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

TARGET_BOTH     = 1
NOT_UNICODE     = 1
PRECOMPILED_CXX = 1

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME      = cabview
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
TARGETEXT       = dll

DLLDEF          = ..\cabview.def

!IF $(ALPHA)
USE_NTDLL       = 1
!ELSE
USE_NOLIBS      = 1
!ENDIF

!if $(IA64) || $(AXP64)
USE_LIBCMT      = 1
!else
USE_IERT        = 1
!endif

TARGETLIBS      = $(BASEDIR)\public\sdk\lib\*\kernel32.lib      \
                  $(BASEDIR)\public\sdk\lib\*\gdi32p.lib        \
                  $(BASEDIR)\public\sdk\lib\*\user32p.lib       \
                  $(BASEDIR)\public\sdk\lib\*\advapi32.lib      \
                  $(BASEDIR)\public\sdk\lib\*\comdlg32.lib      \
                  $(BASEDIR)\public\sdk\lib\*\mpr.lib           \
                  $(BASEDIR)\public\sdk\lib\*\shlwapi.lib       \
                  $(BASEDIR)\public\sdk\lib\*\shell32.lib       \
                  $(BASEDIR)\public\sdk\lib\*\shell32p.lib      \
                  $(BASEDIR)\public\sdk\lib\*\ole32.lib         \
                  $(BASEDIR)\public\sdk\lib\*\uuid.lib          \
                  $(BASEDIR)\public\sdk\lib\*\comctl32.lib      \
                  $(BASEDIR)\public\sdk\lib\*\comctlp.lib       \
                  $(BASEDIR)\public\sdk\lib\*\shlwapip.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib            \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib           \
                  $(CCSHELL_DIR)\lib\$(O)\stocklib.lib

DLLBASE 	= 0x71200000

!if $(WIN64)
DLLENTRY	= _DllMainCRTStartup
!else
DLLENTRY	= DllMain
!endif

SOURCES         = ..\cabview.rc  \
                  ..\cabitms.cpp \
                  ..\dataobj.cpp \
                  ..\debug.c     \
                  ..\dlg.cpp     \
                  ..\enum.cpp    \
                  ..\fdidll.c    \
                  ..\folder.cpp  \
                  ..\icon.cpp    \
                  ..\menu.cpp    \
                  ..\thisdll.cpp \
                  ..\unknown.cpp \
                  ..\xicon.cpp

PRECOMPILED_INCLUDE = ..\pch.h


# Make the CAB after propagating the DLL.  We don't want to make the
# cab until after the propagation, so that we have a chance to rebase
# and strip the EXEs.  So propagate all these files, including a script
# that the buildlab will run to create the cab.

MISCFILES       = ..\cabview.inf

