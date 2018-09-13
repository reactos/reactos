#
# Common makefile include for this project
#

TARGET_BOTH     = 1

# We don't define _ATL_MIN_CRT because we implement the functions ourselves
# (either in crtfree.h or iert.lib).

# Define _MERGE_PROXYSTUB to merge the proxy/stub code into the DLL
#C_DEFINES       = $(C_DEFINES) -D_MERGE_PROXYSTUB

                  
# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF       
!endif

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

LINKER_FLAGS    = $(LINKER_FLAGS) -MERGE:.CRT=.data

CONDITIONAL_INCLUDES = \
                  macocidl.h \
                  version.h
