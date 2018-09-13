#  HOTMAIL.INC
#
#  -- root include file for the hotmail project.  should be !included
#     before any macros are set, etc, with the exception of HMROOT,
#     which is required here
#
#  History:
#     22-March-98  Created by Shayne Bradley (shayneb)

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
!ifndef WIN16
WINVER_VERSION=0x0400
!endif

#ALT_PROJECT_TARGET=hotmail
#BINPLACE_PLACEFILE=$(HMROOT)\placefil.txt

INCLUDES=$(HMROOT)\external\inc;$(HMROOT)\inc

# treat warnings as errors
MSC_WARNING_LEVEL=/W3 /WX

# NOTE: the NT_BUILD below is a temp hack to allow old headers
# to function correctly with split windows headers

C_DEFINES=-DSTRICT -DNT_BUILD -DNASHVILLE -DWINVER=0x0400

!ifdef HMBROWSE
BROWSER_INFO=1
BSCMAKE_FLAGS=/n /nologo
!endif

!if !$(FREEBUILD)
LINKER_NOICF=1
C_DEFINES=$(C_DEFINES) -DDEBUG
!endif

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

