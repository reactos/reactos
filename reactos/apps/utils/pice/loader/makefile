#CC = gcc
#CFLAGS := -g -Wall -fomit-frame-pointer -O2
#LDFLAGS := -dynamic
#-lncurses

#OBJS = main.o terminal.o
#

#loader: 	$(OBJS)
#		$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

#clean:
#		@echo deleting all intermediate files
#		@rm $(OBJS) loader -f
#		make


PATH_TO_TOP = ../../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = loader

TARGET_CFLAGS = -g -fomit-frame-pointer

TARGET_SDKLIBS =

TARGET_OBJECTS = main.o terminal.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

