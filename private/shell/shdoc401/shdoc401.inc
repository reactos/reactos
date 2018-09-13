#
# Common makefile include for this project
#

TARGET_BOTH     = 1
NOT_UNICODE     = 1

#
#   SHDOC401_DLL    = "This is SHDOC401"
#   SHDOC401_DLL_UI = "I want IE 4.01 style UI, no new fancy features"
#
#   Separating it allows UI designers to decide later on, "Well okay,
#   maybe we should pick up this (or that) UI feature in SHDOC401."
#
C_DEFINES = $(C_DEFINES) -DSHDOC401_DLL -DSHDOC401_DLL_UI

#
#   IE4.01 had channels, so we must also.
#
C_DEFINES = $(C_DEFINES) -DENABLE_CHANNELS -DMLUI_SUPPORT

##C_DEFINES       = $(C_DEFINES) \
#                  -DIN_SHDOCVW \
#                  -DFEATURE_FRAMES \
#                  -DMSOCT2=1 \
#                  -DBETA1_DIALMON_HACK \
#                  -DFEATURE_URLHIST \
#                  -DFEATURE_PICS \
#                  -DPAGER \
#                  -D_HSFOLDER

# Defines for deskmover
C_DEFINES       = $(C_DEFINES) \
                  -D_NTSDK -D_USRDLL -D_DESK_MOVER

#!if "$(TEST_AMBIENTS)" == "1"
#C_DEFINES       = $(C_DEFINES) -DTEST_AMBIENTS
#!endif

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!endif

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

USE_STATIC_ATL  = 1

LINKER_FLAGS    = $(LINKER_FLAGS) -MERGE:.CRT=.data
#DELAYLOAD = OLE32.DLL;NTDLL.DLL

# We need SDK_LIB_PATH so we can pull in shldisp.tlb
INCLUDES        = $(CCSHELL_DIR)\shell32;$(INCLUDES);$(SDK_LIB_PATH)

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                    statreg.h       \
                    statreg.cpp     \
                    unicpp\clsobj.tbl \
                    
