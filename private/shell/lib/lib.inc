#
# Common makefile include for this project
#


# BUGBUG (scotth) 11/4/98: once our asserts are cleaned up, we'll turn this on all 
#                          the time, even for NT's checked builds.
!if "$(BUILD_PRODUCT)" == "IE" || "$(SHELLDEV)" == "1"
C_DEFINES   = $(C_DEFINES) -DBREAK_ON_ASSERTS
!endif

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc
