#
#  Common settings for explorer
#

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

#
# Need hydra headers only on NT
#
!if !defined(TARGET_WIN95)
INCLUDES        = $(INCLUDES);$(BASEDIR)\public\sdk\inc\hydra
!endif

# A stupid application loads Explorer.exe as a library and this fails
# if relocations are not present. So for compat reasons we want fixups.
# App Name: Seagate Backup
LINKER_FLAGS    = $(LINKER_FLAGS) -FIXED:NO
DELAYLOAD       = OLE32.DLL;SHELL32.DLL

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

