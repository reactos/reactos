PATH_TO_TOP = ../../../..

TARGET_TYPE = export_driver

TARGET_NAME = usbcore

TARGET_DDKLIBS = ntoskrnl.a

TARGET_CFLAGS = -Wall -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_OBJECTS = \
  message.o hcd.o hcd-pci.o hub.o usb.o config.o urb.o \
  buffer_simple.o usb-debug.o ../sys/ros_wrapper.o \
  ../sys/linuxwrapper.o usbcore.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
