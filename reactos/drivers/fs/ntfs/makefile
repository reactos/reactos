# $Id$

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = ntfs

TARGET_CFLAGS = -Werror -Wall

TARGET_OBJECTS = $(TARGET_NAME).o attrib.o blockdev.o create.o dirctl.o \
                 fcb.o finfo.o fsctl.o mft.o volinfo.o close.o rw.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
