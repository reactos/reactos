#
# Common makefile include for this project
#

TARGET_BOTH     = 1

C_DEFINES       = $(C_DEFINES) \
                  -DIN_SHDOCVW \
                  -DFAVORITESTOSHDOCVW \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!endif

# We don't define _ATL_MIN_CRT because we implement the functions ourselves
# (either in crtfree.h or iert.lib).

# Define _MERGE_PROXYSTUB to merge the proxy/stub code into the DLL
#C_DEFINES       = $(C_DEFINES) -D_MERGE_PROXYSTUB

# Bring in the ATL headers
USE_STATIC_ATL = 1

!include $(CCSHELL_DIR)\common.inc

LINKER_FLAGS    = $(LINKER_FLAGS) -MERGE:.CRT=.data
#DELAYLOAD =

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

# 
# Skip these files when checking for dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  emdef.h    \
                  emrule.h   \
                  mso.h      \
                  msoem.h    \
                  statreg.h  \
                  statreg.cpp \
                  
