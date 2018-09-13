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
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

NTLEANANDMEAN=

MAJORCOMP=shell
MINORCOMP=grpconv

TARGETNAME=grpconv
TARGETPATH=obj
TARGETTYPE=PROGRAM
TARGETLIBS=

NOT_LEAN_AND_MEAN=1

# this is an OS component and therefore it needs the OS version stamp
USE_NT_PRODUCT_VER=1

CCSHELL_DIR     = $(BASEDIR)\private\shell

!include $(CCSHELL_DIR)\shell32\shell32.inc

!if defined(TARGET_WIN95)
USE_LIBCMT      = 1
!else
# Need to use libc.lib for NT version for now
!undef USE_NOLIBS
!endif

!if defined(TARGET_WIN95)
C_DEFINES=$(C_DEFINES)
!else
C_DEFINES=$(C_DEFINES) -DNT -DWINNT -DUNICODE
!endif

# Make warnings equivalent to errors
!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=.;..;$(BASEDIR)\private\shell\inc;$(BASEDIR)\private\inc;$(BASEDIR)\private\windows\inc

SOURCES= \
         ..\gcinst.c \
         ..\group.c  \
         ..\util.c   \
         ..\init.c

UMTYPE=windows
!IF $(386)
UMENTRYABS=ModuleEntry@0
!ELSE
UMENTRYABS=ModuleEntry
!ENDIF
UMRES=$(O)\grpconv.res

!if defined(TARGET_WIN95)
UMLIBS= \
        $(BASEDIR)\public\sdk\lib\chicago\kernel32.lib         \
        $(BASEDIR)\public\sdk\lib\chicago\gdi32.lib            \
        $(BASEDIR)\public\sdk\lib\chicago\*\user32.lib         \
        $(BASEDIR)\public\sdk\lib\chicago\*\advapi32.lib       \
        $(BASEDIR)\public\sdk\lib\chicago\*\comctlp.lib        \
        $(BASEDIR)\private\lib\chicago\*\shell32.w95     \
        $(BASEDIR)\public\sdk\lib\*\version.lib                \

!else
UMLIBS= \
        $(SDK_LIB_PATH)\kernel32.lib       \
        $(SDK_LIB_PATH)\gdi32.lib          \
        $(SDK_LIB_PATH)\user32.lib         \
        $(SDK_LIB_PATH)\advapi32.lib       \
        $(SDK_LIB_PATH)\comctlp.lib        \
        $(SDK_LIB_PATH)\version.lib        \
!if $(386)	
        $(BASEDIR)\private\lib\*\shell32.nt4
!else	
        $(SDK_LIB_PATH)\shell32p.lib
!endif	

!endif

UMLIBS = $(UMLIBS) \
        $(BASEDIR)\private\shell\lib\$(O)\shguidp.lib \
        $(BASEDIR)\private\shell\lib\$(O)\shguid.lib  \

PRECOMPILED_INCLUDE=..\grpconv.h
PRECOMPILED_PCH=grpconv.pch
PRECOMPILED_OBJ=grpconv.obj
