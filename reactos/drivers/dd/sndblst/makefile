PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_CFLAGS = -Wall #-Werror

TARGET_TYPE = driver

TARGET_NAME = sndblst

TARGET_OBJECTS = sndblst.o \
		portio.o \
		settings.o \
		dma.o \
		irq.o \
		card.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
