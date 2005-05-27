PATH_TO_TOP = ../../../..

TARGET_TYPE = export_driver

TARGET_NAME = uhci

TARGET_DDKLIBS = ntoskrnl.a usbcore.a

TARGET_CFLAGS = -Wall -I$(PATH_TO_TOP)/ntoskrnl/include -DDEBUG_MODE

TARGET_OBJECTS = \
  uhci-hcd.o uhci_main.o ../sys/ros_wrapper.o ../sys/linuxwrapper.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
