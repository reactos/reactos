#
# Common makefile include for this project
#

!include $(CCSHELL_DIR)\common.inc

C_DEFINES       = $(C_DEFINES) \
                  -DMLUI_SUPPORT \
                  -DMLUI_MESSAGEBOX \

SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

