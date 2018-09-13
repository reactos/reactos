#
#  Common settings for shlwapi
#

C_DEFINES       = $(C_DEFINES) -DDBCS

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                new

# The Hydra header files are screwed up - you can't include them with
# "#include <hydra\winsta.h>" because they assume they are in sdk\inc,
# so we will add sdk\inc\hydra to the INCLUDES so they sort of work.

INCLUDES=$(INCLUDES);$(SDK_INC_PATH)\hydra
