#
# Common makefile include for this project
#

#NOT_UNICODE     = 1
TARGET_BOTH     = 1

C_DEFINES       = $(C_DEFINES) \
                  -DIN_SHDOCVW \
                  -DFEATURE_FRAMES \
                  -DMSOCT2=1 \
                  -DBETA1_DIALMON_HACK \
                  -DFEATURE_URLHIST \
                  -DFEATURE_PICS \
                  -DPAGER \
                  -D_HSFOLDER \
                  -DFAVORITESTOSHDOCVW \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \
                  -DWINDOWS_ME

# Defines for deskmover
C_DEFINES       = $(C_DEFINES) \
                  -D_NTSDK -D_USRDLL -D_WINNT

!if "$(TEST_AMBIENTS)" == "1"
C_DEFINES       = $(C_DEFINES) -DTEST_AMBIENTS
!endif

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
DELAYLOAD = OLE32.DLL

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

# common.inc has a whole list of common header files that are 
# added to CONDITIONAL_INCLUDES.
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) 

