PATH_TO_TOP := ../../../..
TARGET_TYPE := dynlink
TARGET_NAME := notifyhook
TARGET_CFLAGS := -D__USE_W32API -D_WIN32_IE=0x0600 -Wall -D_NOTIFYHOOK_IMPL
TARGET_NORC := yes
TARGET_DEFONLY := yes
TARGET_OBJECTS := notifyhook.o
TARGET_BASE = $(TARGET_BASE_LIB_NOTIFYHOOK)

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
