# $Id$
#
#  ReactOS makefile for RegExpl
#

PATH_TO_TOP := ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = regexpl

TARGET_OBJECTS = 			\
	ArgumentParser.o		\
	Console.o			\
	RegistryExplorer.o		\
	RegistryKey.o			\
	RegistryTree.o			\
	SecurityDescriptor.o		\
	ShellCommand.o			\
	ShellCommandChangeKey.o		\
	ShellCommandConnect.o		\
	ShellCommandDACL.o		\
	ShellCommandDeleteKey.o		\
	ShellCommandDeleteValue.o	\
	ShellCommandDir.o		\
	ShellCommandExit.o		\
	ShellCommandHelp.o		\
	ShellCommandNewKey.o		\
	ShellCommandOwner.o		\
	ShellCommandSACL.o		\
	ShellCommandSetValue.o		\
	ShellCommandValue.o		\
	ShellCommandVersion.o		\
	ShellCommandsLinkedList.o	\
	CrtSupplement.o			\
	TextHistory.o			\
	Completion.o			\
	Pattern.o			\
	Settings.o			\
	Prompt.o			

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DUNICODE -D_UNICODE -I./

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# C++ code should be linked with c++ aware linker (g++)
LD_CC = $(CXX)

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/tools/depend.mk

# EOF
