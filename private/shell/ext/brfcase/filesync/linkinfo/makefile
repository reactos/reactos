############################
# makefile for LinkInfo DLL
############################

!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

#
# Build using NMAKE.EXE
#

#
# Set default build.
#
# $(BUILD) may be:
#
#     debug    -> debug Win32
#     retail   -> retail Win32
#

!ifndef BUILD
BUILD             = debug
!endif

#
# Set directories.
#

ROOT              = ..\..\..
SHARED_DIR        = $(ROOT)\rover\filesync\core

#
# Set output base name.
#

OUT_NAME          = linkinfo

#
# Set tools' paths.
#

HC                = hc31

#
# Set libraries.
#

LIBS              = kernel32.lib user32.lib advapi32.lib mpr.lib

#
# Set file macros.
#

ASM_SRC           =
ASM_OBJ           =

C_SRC             = dllinit.c\
                    volumeid.c\
                    server.c\
                    cnrlink.c\
                    canon.c\
                    util.c\
                    linkinfo.c

C_OBJ             = $(DEST_DIR)\dllinit.obj\
                    $(DEST_DIR)\volumeid.obj\
                    $(DEST_DIR)\server.obj\
                    $(DEST_DIR)\cnrlink.obj\
                    $(DEST_DIR)\canon.obj\
                    $(DEST_DIR)\util.obj\
                    $(DEST_DIR)\linkinfo.obj

SHARED_C_SRC      = init.c\
                    serial.c\
                    memmgr.c\
                    valid.c\
                    comc.c\
                    inifile.c\
                    resstr.c\
                    debug.c

SHARED_C_OBJ      = $(DEST_DIR)\init.obj\
                    $(DEST_DIR)\serial.obj\
                    $(DEST_DIR)\memmgr.obj\
                    $(DEST_DIR)\valid.obj\
                    $(DEST_DIR)\comc.obj

!if "$(BUILD)" == "debug"

SHARED_C_OBJ      = $(SHARED_C_OBJ)\
                    $(DEST_DIR)\inifile.obj\
                    $(DEST_DIR)\resstr.obj\
                    $(DEST_DIR)\debug.obj

!endif

C_SRC             = $(C_SRC) $(SHARED_C_SRC)

C_OBJ             = $(C_OBJ) $(SHARED_C_OBJ)

#
# Set documentation macros.
#

CANON_DOC_LIST    = canon.c

LINKINFO_DOC_LIST = linkinfo.c volumeid.c cnrlink.c

DOC_LIST          = $(CANON_DOC_LIST) $(LINKINFO_DOC_LIST)

DOCS_DIR          = docs

#
# Set manifest constants.
#

EXT_DEFINES       = -DEXPV -DVSTF


!include $(SHARED_DIR)\common.mk


#######################
# build pseudo-targets
#######################

#
# Build LinkInfo documentation (API descriptions and WinHelp).
#

docs: checkdoc apidescs winhelp


##############
# build rules
##############

#
# Copy shared source code.
#

init.c: $(SHARED_DIR)\init.c
   copy $(SHARED_DIR)\init.c

serial.c: $(SHARED_DIR)\serial.c
   copy $(SHARED_DIR)\serial.c

memmgr.c: $(SHARED_DIR)\memmgr.c
   copy $(SHARED_DIR)\memmgr.c

valid.c: $(SHARED_DIR)\valid.c
   copy $(SHARED_DIR)\valid.c

comc.c: $(SHARED_DIR)\comc.c
   copy $(SHARED_DIR)\comc.c

inifile.c: $(SHARED_DIR)\inifile.c
   copy $(SHARED_DIR)\inifile.c

resstr.c: $(SHARED_DIR)\resstr.c
   copy $(SHARED_DIR)\resstr.c

debug.c: $(SHARED_DIR)\debug.c
   copy $(SHARED_DIR)\debug.c

#
# Create documentation directory.
#

checkdoc:
   @echo ***** docs build *****
   @echo ÿ
   -@md $(DOCS_DIR)

#
# Generate LinkInfo API descriptions.
#

apidescs: $(DOCS_DIR)\canapis.rtf $(DOCS_DIR)\liapis.rtf

$(DOCS_DIR)\canapis.rtf: $(CANON_DOC_LIST)
   $(AUTODOC) -v -xLINKINFOAPI -x INTERNAL -rd -o $@ $**

$(DOCS_DIR)\liapis.rtf: $(LINKINFO_DOC_LIST)
   $(AUTODOC) -v -xLINKINFOAPI -x INTERNAL -rd -o $@ $**

#
# Generate LinkInfo WinHelp file.
#

winhelp: $(DOCS_DIR)\$(OUT_NAME).hlp

$(DOCS_DIR)\$(OUT_NAME).hlp: $(OUT_NAME).hpj $(DOCS_DIR)\$(OUT_NAME).rtf
   $(HC) $(OUT_NAME).hpj
   copy $(OUT_NAME).hlp $(DOCS_DIR)
   del $(OUT_NAME).hlp

$(DOCS_DIR)\$(OUT_NAME).rtf: $(DOC_LIST)
   $(AUTODOC) -v -xLINKINFOAPI -rh -o $@ $**

!endif # NTMAKEENV
