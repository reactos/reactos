# $Id: $

PATH_TO_TOP = ../../..
TARGET_TYPE = library
TARGET_NAME = ext2lib
TARGET_CFLAGS = -I$(W32API_PATH)/include/ddk -D__USE_W32API -Wall -Werror -DDBG
TARGET_OBJECTS = \
	Badblock.o \
	Bitmap.o \
	Disk.o \
	Group.o \
	Inode.o \
	Memory.o \
	Mke2fs.o \
	Super.o \
	Uuid.o
 
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
