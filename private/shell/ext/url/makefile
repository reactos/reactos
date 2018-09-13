###############################################################################
#
#  Microsoft Confidential
#  Copyright (C) Microsoft Corporation 1995
#  All Rights Reserved.
#
#  URL Shell extension makefile
#
###############################################################################

!if "$(NTMAKEENV)" != "" 
#
# This section is for the NT build ONLY.
# For NT build, change the 'sources' or 'dirs' file, not this makefile
#
clean:

!else
#
# Non-NT build makefile start HERE.
#
####################
# macro definitions
####################

ROOT              = ..\..\..
BASE_NAME         = url
DLL               = 1

CORE_DIR          = $(ROOT)\win
SHELL_DIR         = $(CORE_DIR)\shell
SHELLDLL_DIR      = $(SHELL_DIR)\shelldll

!ifndef BUILD
BUILD             = debug
!endif

LIBS              = kernel32.lib gdi32.lib user32.lib advapi32.lib shlwapi.lib shell32.lib comctl32.lib
LIBS              = $(LIBS) libcmt.lib
LIBS              = $(LIBS) uuid.lib

ENTRY_POINT       = DllMain

EXT_C_SWITCHES    = -Gz -W3

EXT_DEFINES       = -D_MT \
                    -DEXPV \
                    -DMIME

!if "$(BUILD)" == "debug"
EXT_DEFINES       = $(EXT_DEFINES) \
                    -DVSTF
!endif

EXT_DEFINES       = $(EXT_DEFINES:  = )

EXT_INCLUDE       = $(CORE_DIR)\inc

EXT_TARGETS       = $(DEST_DIR)\$(OUT_NAME).lib \
                    $(DEST_DIR)\$(OUT_NAME).rlb

ASM_SRC           =

ASM_OBJ           =

URL_C_SRC         = assoc.c \
                    comc.c \
                    debspew.c \
                    filetype.c \
                    fsassoc.c \
                    inetcpl.c \
                    inifile.c \
                    init.c \
                    memmgr.c \
                    olevalid.c \
                    resstr.c \
                    serial.c \
                    isguids.c \
                    shlvalid.c \
                    util.c \
                    valid.c

#SHELLDLL_C_SRC    = $(SHELLDLL_DIR)\hash.c

#C_SRC             = $(URL_C_SRC) $(SHELLDLL_C_SRC)
C_SRC             = $(URL_C_SRC)


C_SRC             = $(C_SRC:  = )

URL_C_OBJ         = $(DEST_DIR)\assoc.obj \
                    $(DEST_DIR)\comc.obj \
                    $(DEST_DIR)\filetype.obj \
                    $(DEST_DIR)\fsassoc.obj \
                    $(DEST_DIR)\inetcpl.obj \
                    $(DEST_DIR)\init.obj \
                    $(DEST_DIR)\memmgr.obj \
                    $(DEST_DIR)\serial.obj \
                    $(DEST_DIR)\isguids.obj \
                    $(DEST_DIR)\util.obj \
                    $(DEST_DIR)\valid.obj

!if "$(BUILD)" == "debug"

URL_C_OBJ         = $(URL_C_OBJ) \
                    $(DEST_DIR)\debspew.obj \
                    $(DEST_DIR)\inifile.obj \
                    $(DEST_DIR)\olevalid.obj \
                    $(DEST_DIR)\resstr.obj \
                    $(DEST_DIR)\shlvalid.obj

!endif

SHELLDLL_C_OBJ    = $(DEST_DIR)\hash.obj

C_OBJ             = $(URL_C_OBJ) \
                    $(SHELLDLL_C_OBJ)

C_OBJ             = $(C_OBJ:  = )

CPP_SRC           = autodial.cpp \
                    comcpp.cpp \
                    contmenu.cpp \
                    dataobj.cpp \
                    dllinit.cpp \
                    enumfmte.cpp \
                    extricon.cpp \
                    ftps.cpp \
                    guids.cpp \
                    inetps.cpp \
                    intshcut.cpp \
                    persist.cpp \
                    propsht.cpp \
                    newshk.cpp \
                    refcount.cpp \
                    shlexhk.cpp \
                    shlink.cpp \
                    url.cpp

CPP_OBJ           = $(DEST_DIR)\autodial.obj \
                    $(DEST_DIR)\comcpp.obj \
                    $(DEST_DIR)\contmenu.obj \
                    $(DEST_DIR)\dataobj.obj \
                    $(DEST_DIR)\dllinit.obj \
                    $(DEST_DIR)\enumfmte.obj \
                    $(DEST_DIR)\extricon.obj \
                    $(DEST_DIR)\ftps.obj \
                    $(DEST_DIR)\guids.obj \
                    $(DEST_DIR)\inetps.obj \
                    $(DEST_DIR)\intshcut.obj \
                    $(DEST_DIR)\persist.obj \
                    $(DEST_DIR)\propsht.obj \
                    $(DEST_DIR)\newshk.obj \
                    $(DEST_DIR)\refcount.obj \
                    $(DEST_DIR)\shlexhk.obj \
                    $(DEST_DIR)\shlink.obj \
                    $(DEST_DIR)\url.obj

SRC               = $(ASM_SRC) $(C_SRC) $(CPP_SRC)

#
# Set documentation macros.
#

INTSHCUT_DOC_LIST = intshcut.txt

DOC_LIST          = $(INTSHCUT_DOC_LIST)

DOCS_DIR          = docs


#################
# common headers
#################

!include $(ROOT)\inet\ohare\ohare.mk
!include $(ROOT)\inet\ohare\win32.mk


#######################
# build pseudo-targets
#######################

#
# Build Internet Shortcut documentation (API descriptions and WinHelp).
#

docs: checkdoc apidescs winhelp


##############
# build rules
##############

#
# Build stolen Shell modules.
#

$(DEST_DIR)\filetype.obj: filetype.c
   @$(CL) @<<
$(CL_SWITCHES) -W3 $(DEFINES) -Yu$(PCH_C_INCLUDE) -Fp$(PCH_C_PCH) -Fo$@ filetype.c
<<

$(DEST_DIR)\fsassoc.obj: fsassoc.c
   @$(CL) @<<
$(CL_SWITCHES) -W3 $(DEFINES) -USTRICT -Fo$@ fsassoc.c
<<

$(DEST_DIR)\hash.obj: hash.c
   @$(CL) @<<
$(CL_SWITCHES) -W3 $(DEFINES) -USTRICT -Fo$@ hash.c
<<

#
# Add explicit resource dependency on icons.
#

$(DEST_DIR)\$(BASE_NAME).res: webdoc.ico

#
# Add GUID object file to private import library.
#

$(DEST_DIR)\$(OUT_NAME).lib: $(DEST_DIR)\isguids.obj
   $(LINK) -lib -out:$@ $@ $(DEST_DIR)\isguids.obj

#
# Preprocess PDK import library module definition file.
#

$(DEST_DIR)\$(BASE_NAME).rdf: $(BASE_NAME).def
   $(CL) -nologo -EP $(DEFINES) -DPDK -Tc$(BASE_NAME).def > $@

#
# Create PDK import library, and add GUID object file.
#

$(DEST_DIR)\$(OUT_NAME).rlb: $(OBJ_LIST) $(DEST_DIR)\$(BASE_NAME).rdf $(DEST_DIR)\isguids.obj
   $(LINK) -lib @<<
-def:$(DEST_DIR)\$(BASE_NAME).rdf
-machine:ix86
$(OBJ_LIST)
-out:$@
<<
   $(LINK) -lib -out:$@ $@ $(DEST_DIR)\isguids.obj

#
# Create documentation directory.
#

checkdoc: banner
   @if not exist $(DOCS_DIR)\nul mkdir $(DOCS_DIR)

#
# Generate Internet Shortcut interface and API descriptions.
#

apidescs: $(DOCS_DIR)\isifaces.rtf $(DOCS_DIR)\isapis.rtf $(DOCS_DIR)\mimeapis.rtf

$(DOCS_DIR)\isifaces.rtf: $(INTSHCUT_DOC_LIST)
   $(AUTODOC) -rd -x INTSHCUTIFACE -o $@ $**

$(DOCS_DIR)\isapis.rtf: $(INTSHCUT_DOC_LIST)
   $(AUTODOC) -rd -x INTSHCUTAPI -o $@ $**

$(DOCS_DIR)\mimeapis.rtf: $(INTSHCUT_DOC_LIST)
   $(AUTODOC) -rd -x MIMEAPI -o $@ $**

#
# Generate Internet Shortcut WinHelp file.
#

winhelp: $(DOCS_DIR)\intshcut.hlp

$(DOCS_DIR)\intshcut.hlp: $(DOCS_DIR)\intshcut.hpj $(DOCS_DIR)\intshcut.rtf
   $(HC) $(DOCS_DIR)\intshcut.hpj
   $(MOVE) intshcut.hlp $(DOCS_DIR)

$(DOCS_DIR)\intshcut.hpj $(DOCS_DIR)\intshcut.rtf: $(DOC_LIST)
   $(AUTODOC) -rh -x INTSHCUTIFACE -x INTSHCUTAPI -x MIMEAPI -o $(DOCS_DIR)\intshcut.rtf $**

!endif    # !if NTMAKEENV
