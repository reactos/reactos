# $Id: makefile,v 1.3 2003/04/05 09:37:42 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = ntfs

TARGET_OBJECTS = $(TARGET_NAME).o attrib.o blockdev.o create.o dirctl.o \
                 fcb.o finfo.o fsctl.o mft.o volinfo.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
