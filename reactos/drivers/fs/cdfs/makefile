# $Id: makefile,v 1.6 2003/11/10 11:32:08 ekohl Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = driver

TARGET_NAME = cdfs

TARGET_OBJECTS = $(TARGET_NAME).o close.o common.o create.o dirctl.o \
                 fcb.o finfo.o fsctl.o misc.o rw.o volinfo.o cleanup.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
