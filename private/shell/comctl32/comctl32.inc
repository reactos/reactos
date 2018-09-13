#
#  Common settings for comctl32
#

USE_LEGO        = 1

# Now the NT version have FE_IME etc. defined
C_DEFINES       = $(C_DEFINES) -DFE_IME -DDBCS -DWINDOWS_ME 

!if !defined(TARGET_WIN95)
C_DEFINES       = $(C_DEFINES) -DUSE_PRIVATE_VERSION_NUMBER
!else
C_DEFINES       = $(C_DEFINES) -DUNICODE
!endif

C_DEFINES       = $(C_DEFINES) -DFONT_LINK

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif


!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                mwavi.h         \
