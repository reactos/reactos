#
# Common makefile include for this project
#

C_DEFINES       = $(C_DEFINES) \
                  -DEXPV \
                  -DMIME \
                  -DBUILDDLL \
                  -DIEXPLORER_DOESNT_CHECK_HEAP_RETURN_VALUES

!IF !$(FREEBUILD)
C_DEFINES       = $(C_DEFINES) \
                  -DVSTF
!ENDIF

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc
DELAYLOAD       =  WININET.DLL

