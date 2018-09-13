#
#  Common settings for shell32
#

C_DEFINES       = $(C_DEFINES) -DSYNC_BRIEFCASE -DMIME -DDBCS

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

!if !defined(TARGET_WIN95)
#  (We don't want this defined if we're building for Win95)
C_DEFINES       = $(C_DEFINES) -DPARTIAL_UNICODE  -DPRN_FOLDERDATA -DWINDOWS_ME
!endif

# Defines for deskmover
C_DEFINES       = $(C_DEFINES) \
                  -D_NTSDK -D_DESK_MOVER

# Uncomment the next line to turn on POSTSPLIT
C_DEFINES       = $(C_DEFINES) -DPOSTSPLIT


INCLUDES        = $(CCSHELL_DIR)\shell32;$(INCLUDES);$(SDK_LIB_PATH);$(BASEDIR)\public\sdk\inc\hydra

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

USE_STATIC_ATL  = 1

DELAYLOAD       = WININET.DLL

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  filter.h      \
                  rcids.h       \
                  tracker.h     \
                  ver.h         \
                  unicpp\clsobj.tbl \
                  
