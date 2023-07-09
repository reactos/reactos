# First, define some root variables for intra project references.
# By default, these are:
#
# LANGAPI_ROOT      $(VCROOT)\langapi
# CRT_ROOT          $(VCROOT)\crt
# PDBSRC_ROOT       $(VCROOT)\pdb
# IDE_ROOT          $(VCROOT)\ide
# DEBUGGER_ROOT     $(IDE_ROOT)\debugger
# VCPUBLIC          $(VCROOT)\public

!ifndef VCTOOLS
VCTOOLS=$(BASEDIR)\private\windbg64
!endif

!ifndef LANGAPI_ROOT
LANGAPI_ROOT=$(VCTOOLS)\langapi
!endif
LANGAPI=$(LANGAPI_ROOT)

!ifndef CRT_ROOT
CRT_ROOT = $(VCROOT)\BoGuS
!endif

!ifndef PDBSRC_ROOT
PDBSRC_ROOT = $(VCTOOLS)\pdb
!endif

!ifndef DEBUGGER_ROOT
DEBUGGER_ROOT = $(VCTOOLS)\debugger
!endif

!ifndef VCPUBLIC
VCPUBLIC = $(VCROOT)\BoGuS
!endif

!ifndef NONSHIP_LIB_PATH
NONSHIP_LIB_PATH = $(BASEDIR)\public\sdk\lib\*
!endif

!ifndef NONSHIP_INC_PATH
NONSHIP_INC_PATH = $(BASEDIR)\public\sdk\inc
!endif

!ifndef MFC_LIBDIR
MFC_LIBDIR=$(SDK_LIB_PATH)
!endif


# !IFDEF DEBUGVER
# USE_RTTI=1
# !ENDIF

# USECXX_FLAG=/TP


!ifndef OSDEBUG4
OSDEBUG4=1
!endif

# turn on 32-bit types everywhere with one swell foop.
# 386_CPPFLAGS = -DCC_CVTYPE32 -DCC_LAZYTYPES

C_DEFINES = $(C_DEFINES) -DNT_BUILD_ONLY -DNT_BUILD -DCC_CVTYPE32 -DCC_LAZYTYPES /DNOCRYPT


!ifndef DEBUGGER_BUILD
DEBUGGER_BUILD=1
!endif

MASTER_VERSION_FILE = $(LANGAPI_ROOT)\include\version.h

# Then define the SDK, MFC and CRT paths needed to build VC
# (these are different from the paths used to build NT)

DBC=1

NEW_CRTS = 1

!ifndef MFC_VER
MFC_VER = 42
!endif


!UNDEF DEBUG_CRTS

!IF $(FREEBUILD)
! IFDEF USE_MFCUNICODE
MFC_LIBS= \
    $(MFC_LIBDIR)\mfc$(MFC_VER)u.lib \
    $(MFC_LIBDIR)\mfcs$(MFC_VER)u.lib
! ELSE
MFC_LIBS= \
    $(MFC_LIBDIR)\mfc$(MFC_VER).lib \
    $(MFC_LIBDIR)\mfcs$(MFC_VER).lib
! ENDIF

!ELSE

#DEBUG_CRTS=1
! IFDEF USE_MFCUNICODE
MFC_LIBS= \
    $(MFC_LIBDIR)\mfc$(MFC_VER)ud.lib \
    $(MFC_LIBDIR)\mfcs$(MFC_VER)ud.lib \
    $(MFC_LIBDIR)\mfco$(MFC_VER)ud.lib \
    $(MFC_LIBDIR)\mfcd$(MFC_VER)ud.lib \
    $(MFC_LIBDIR)\mfcn$(MFC_VER)ud.lib
! ELSE
MFC_LIBS= \
    $(MFC_LIBDIR)\mfc$(MFC_VER)d.lib \
    $(MFC_LIBDIR)\mfcs$(MFC_VER)d.lib \
    $(MFC_LIBDIR)\mfco$(MFC_VER)d.lib \
    $(MFC_LIBDIR)\mfcd$(MFC_VER)d.lib \
    $(MFC_LIBDIR)\mfcn$(MFC_VER)d.lib
! ENDIF
!ENDIF

NO_INCREMENTAL_LINKING=1

# Get all the other project paths.

DBG_ROOT=$(VCTOOLS)\debugger

NOT_LEAN_AND_MEAN=

USE_MSVCRT = 1

DLLENTRY = _DllMainCRTStartup

MSC_WARNING_LEVEL=/WX /W3

C_DEFINES = $(C_DEFINES) -DWIN32 -DTARGET32 -DHOST32 -D_CROSS_PLATFORM_ -DOSDEBUG4
C_DEFINES = $(C_DEFINES) -DCC_CVTYPE32 -DCC_LAZYTYPES

386_FLAGS  = $(386_FLAGS)   -DHOST_i386
ALPHA_FLAGS= $(ALPHA_FLAGS) -DHOST_ALPHA
AXP64_FLAGS= $(AXP64_FLAGS) -DHOST_AXP64
IA64_FLAGS = $(IA64_FLAGS)  -DHOST_IA64

!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES) -DNDEBUG
D=
!else
C_DEFINES = $(C_DEFINES) -DDEBUGVER
D=
!endif

DEBUG_INCLUDES = \
                $(NONSHIP_INC_PATH);\
                $(LANGAPI_ROOT)\include;\
                $(LANGAPI_ROOT)\debugger;\
                $(LANGAPI_ROOT)\include

DEBUG_LIB = $(DBG_ROOT)\lib

SUBSYSTEM_VERSION=4.00

