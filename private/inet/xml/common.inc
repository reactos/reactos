##########################################################################
#
#   Microsoft Confidential
# Copyright (c) 1991 - 1999 Microsoft Corporation. All rights reserved.
#
##########################################################################

#
# Common include file 'sources' files in the xml project.
#
#

MSXML=$(ROOT)

WIN32_IE_VERSION=0x0500
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
SUBSYSTEM_VERSION=4.00

# Treat warnings as errors
MSC_WARNING_LEVEL=/W3 /WX

# Set this to 0 to build MSXMLDBG as a DLL
BUILD_XMLDBG_AS_LIB=1

USE_NOLIBS=1
#USE_ICECAP=1

#MEMSTRESS_ENABLE=1

STRUCTURED_EXCEPTION=1

UMTYPE=windows
XML_HTTP_FEATURE=1

!if $(AXP64)
!elseif $(IA64)
!else
SMALLBLOCKHEAP=1
SPECIAL_OBJECT_ALLOCATION=1
!endif

MPHEAP=1

IDL_GENERATED_HEADERS=$(ROOT)\idl\obj

C_DEFINES = \
    /DSTRONGREFATOM\
    /DBINARY_XML\
    /DRENTAL_MODEL\
!ifdef SPECIAL_OBJECT_ALLOCATION
    /DSPECIAL_OBJECT_ALLOCATION=1\
!endif
    /DFAST_OBJECT_LIST=1\
!ifdef MEMSTRESS_ENABLE
    /DMEMSTRESS_ENABLE=1\
!endif
!ifdef PRFDATA
    /DPRFDATA=1\
!endif
!ifdef NEWGC
    /DNEWGC=1\
!else
    /DNEWGC=0\
!endif
!ifdef USE_ICECAP
    /DPRODUCT_PROF=1\
!else
    /DPRODUCT_PROF=0\
!endif
!ifdef MPHEAP
    /DMPHEAP=1\
!else
    /DMPHEAP=0\
!endif
!ifdef SMALLBLOCKHEAP
    /DSMALLBLOCKHEAP=1\
!else
    /DSMALLBLOCKHEAP=0\
!endif
    /DSTRICT \
!ifndef WIN16
    /DUNICODE \
    /D_UNICODE \
    /DOLEDBVER=0x0200\
    /DNASHVILLE \
    /DACTIVEX_DEBUG \
!endif
!ifdef STRUCTURED_EXCEPTION
    /DSTRUCTURED_EXCEPTION \
!endif
!ifdef MSXML_EXPORT
    /DMSXML_EXPORT \
!endif
!ifdef XML_HTTP_FEATURE
    /DXML_HTTP_FEATURE\
!endif
!ifdef BUILD_XMLDBG_AS_LIB
    /DBUILD_XMLDBG_AS_LIB 
!endif

!ifndef WIN16
# WINDOWS_INC_PATH=$(BASEDIR)\private\windows\inc
WINDOWS_INC_PATH=$(BASEDIR)\private\iedev\shell\inc
!else
WINDOWS_INC_PATH=.
!endif

!ifndef SDK_INC_PATH
SDK_INC_PATH=$(BASEDIR)\public\sdk\inc
!endif

MIMEVIEWER_LIB=$(ROOT)\mimeviewer\src\$(O)\mimeview.lib

PASS0_HEADERDIR=$(SDK_INC_PATH)

# Standard name for the MSXML debug library
!if "$(BUILD_XMLDBG_AS_LIB)" == "1"
MAIN_DBG_LIB=xmldbg.lib
!else
MAIN_DBG_LIB=msxmldbg.lib
!endif

MSXML_DEBUG_LIB = \
!if "$(BUILD_XMLDBG_AS_LIB)" == "1"
    $(SDK_LIB_PATH)\gdi32.lib\
!endif
    $(MSXML)\debug\$(O)\$(MAIN_DBG_LIB) \
    $(MSXML)\debug\$(O)\debugpch.lib 

MSXML_DEBUGDLL_LIB = \
    $(MSXML)\debug\$(O)\msxmldbg.lib

PASS0_SOURCEDIR=.

# needs to run on Win95 and NT 3.51
MIDL_OPTIMIZATION=$(MIDL_OPTIMIZATION) -Oicf

MKTYPLIB_FLAGS  = $(MKTYPLIB_FLAGS) /OLD -DWINNT -h $*.h

# Don't generate a .bsc file
!ifdef USE_MSXML_BROWSER_INFO
!message USE_MSXML_BROWSER_INFO was defined
BROWSER_INFO=1
!else
NO_BROWSER_INFO=1
NO_BROWSER_FILE=1
!endif

!if !$(FREEBUILD)
_DEBUG=1
C_DEFINES=$(C_DEFINES) -D_DEBUG=1 /DMSXML_EXPORT
USE_RTTI = 1
MSC_OPTIMIZATION=/Odi /GX- 
MKTYPLIB_FLAGS  = $(MKTYPLIB_FLAGS) -DDEBUG
!else
MSC_OPTIMIZATION=/Oxs /GX- /GR-
!ENDIF

##
## comment out these flags so that we can register the msxml.dll on Win95 machines
##
#!ELSE
#USE_NOLIBS = 1
#USE_IERT   = 1 
#!ENDIF

CHECKED_ALT_DIR=1

!ifdef USE_MSXML_INCREMENTAL_LINKING
!if !$(FREEBUILD) && $(USE_MSXML_INCREMENTAL_LINKING)
NTDEBUG=ntsd
NTDEBUGTYPE=windbg
NTDBGFILES=1
!undef USE_MAPSYM
USE_INCREMENTAL_LINKING=1
LINKER_FLAGS=$(LINKER_FLAGS) -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
TARGETPDB=MSXML
!message USE_MSXML_INCREMENTAL_LINKING was defined
!message $(TARGETPDB) was defined
!endif
!else
USE_MAPSYM=1
!endif

!if $(386)
NO_NTDLL=1
!else
# USE_NTDLL=1
!endif

#LINKER_FLAGS = $(LINKER_FLAGS) -FORCE:UNRESOLVED

CONDITIONAL_INCLUDES=\
    macwin32.h\
    macocidl.h\
    winwlm.h\
    rpcerr.h\
    macname1.h\
    macname2.h\
    macpub.h\
    macapi.h\
    shlwapip.h\
    w95wraps.h\
    rpcmac.h\
    pure.h\
    codepage.h\
    mwconfig.h\
    sys\param.h\
    mwwstdlib.h

